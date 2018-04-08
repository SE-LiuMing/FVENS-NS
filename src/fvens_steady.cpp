#include <iostream>
#include <iomanip>
#include <string>
#include <omp.h>
#include <petscksp.h>

#include "alinalg.hpp"
#include "autilities.hpp"
#include "aoutput.hpp"
#include "aodesolver.hpp"
#include "afactory.hpp"
#include "ameshutils.hpp"

#ifdef USE_BLASTED
#include <blasted_petsc.h>
#endif

using namespace amat;
using namespace acfd;

int main(int argc, char *argv[])
{
	StatusCode ierr = 0;
	const char help[] = "Finite volume solver for Euler or Navier-Stokes equations.\n\
		Arguments needed: FVENS control file and PETSc options file with -options_file.\n";

	ierr = PetscInitialize(&argc,&argv,NULL,help); CHKERRQ(ierr);
	int mpirank;
	MPI_Comm_rank(PETSC_COMM_WORLD, &mpirank);

	// Read control file

	const FlowParserOptions opts = parse_flow_controlfile(argc, argv);

	// Set up mesh

	UMesh2dh m;
	m.readMesh(opts.meshfile);
	CHKERRQ(preprocessMesh(m));
	m.compute_periodic_map(opts.periodic_marker, opts.periodic_axis);

	std::cout << "\n***\n";

	// set up problem
	
	// physical configuration
	const FlowPhysicsConfig pconf = extract_spatial_physics_config(opts);
	// numerics for main solver
	const FlowNumericsConfig nconfmain = extract_spatial_numerics_config(opts);
	// simpler numerics for startup
	const FlowNumericsConfig nconfstart {opts.invflux, opts.invfluxjac, "NONE", "NONE", false};

	std::cout << "Setting up main spatial scheme.\n";
	const Spatial<NVARS> *const prob = create_const_flowSpatialDiscretization(&m, pconf, nconfmain);

	std::cout << "\nSetting up spatial scheme for the initial guess.\n";
	const Spatial<NVARS> *const startprob
		= create_const_flowSpatialDiscretization(&m, pconf, nconfstart);

	std::cout << "\n***\n";

	/* NOTE: Since the "startup" solver (meant to generate an initial solution) and the "main" solver
	 * have the same number of unknowns and use first-order Jacobians, we have just one set of
	 * solution vector, Jacobian matrix, preconditioning matrix and KSP solver.
	 */

	// solution vector
	Vec u;

	// Initialize Jacobian for implicit schemes
	Mat M;
	ierr = setupSystemMatrix<NVARS>(&m, &M); CHKERRQ(ierr);
	ierr = MatCreateVecs(M, &u, NULL); CHKERRQ(ierr);

	// setup matrix-free Jacobian if requested
	Mat A;
	MatrixFreeSpatialJacobian<NVARS> mfjac;
	PetscBool mf_flg = PETSC_FALSE;
	ierr = PetscOptionsHasName(NULL, NULL, "-matrix_free_jacobian", &mf_flg); CHKERRQ(ierr);
	if(mf_flg) {
		std::cout << " Allocating matrix-free Jac\n";
		ierr = setup_matrixfree_jacobian<NVARS>(&m, &mfjac, &A); 
		CHKERRQ(ierr);
	}

	// initialize solver
	KSP ksp;
	ierr = KSPCreate(PETSC_COMM_WORLD, &ksp); CHKERRQ(ierr);
	if(mf_flg) {
		ierr = KSPSetOperators(ksp, A, M); 
		CHKERRQ(ierr);
	}
	else {
		ierr = KSPSetOperators(ksp, M, M); 
		CHKERRQ(ierr);
	}

	// set up time discrization

	const SteadySolverConfig maintconf {
		opts.lognres, opts.logfile+".tlog",
		opts.initcfl, opts.endcfl, opts.rampstart, opts.rampend,
		opts.tolerance, opts.maxiter,
	};

	const SteadySolverConfig starttconf {
		opts.lognres, opts.logfile+"-init.tlog",
		opts.firstinitcfl, opts.firstendcfl, opts.firstrampstart, opts.firstrampend,
		opts.firsttolerance, opts.firstmaxiter,
	};

	const SteadySolverConfig temptconf = {false, opts.logfile, opts.firstinitcfl, opts.firstendcfl,
		opts.firstrampstart, opts.firstrampend, opts.firsttolerance, 1};

	SteadySolver<NVARS> * temptime=nullptr, * starttime=nullptr, * time=nullptr;

	if(opts.timesteptype == "IMPLICIT")
	{
		// Temporal solver context for initial solver setup - needed for PETSc quirks
		temptime = new SteadyBackwardEulerSolver<NVARS>(startprob, temptconf, ksp);

		if(opts.usestarter != 0) {
			starttime = new SteadyBackwardEulerSolver<NVARS>(startprob, starttconf, ksp);
			std::cout << "Set up backward Euler temporal scheme for initialization solve.\n";
		}

	}
	else
	{
		if(opts.usestarter != 0) {
			starttime = new SteadyForwardEulerSolver<NVARS>(startprob, u, starttconf);
			std::cout << "Set up explicit forward Euler temporal scheme for startup solve.\n";
		}
	}

	// Ask the spatial discretization context to initialize flow variables
	startprob->initializeUnknowns(u);

	// One iteration of the initial setup solve
	/*KSPSetType(ksp, KSPGMRES);
	KSPSetTolerances(ksp, 1e-1, 1e-5, 1e4, 1);
	mfjac.set_spatial(startprob);
	ierr = temptime->solve(u); CHKERRQ(ierr);
	delete temptime;*/
	(void)temptime;
	
	ierr = KSPSetFromOptions(ksp); CHKERRQ(ierr);

	// setup BLASTed preconditioning if requested
#ifdef USE_BLASTED
	Blasted_data_vec bctx = newBlastedDataVec();
	if(opts.timesteptype == "IMPLICIT") {
		ierr = setup_blasted<NVARS>(ksp,u,startprob,bctx); CHKERRQ(ierr);
	}
#endif

	std::cout << "\n***\n";

	// computation

	if(opts.usestarter != 0) {

		mfjac.set_spatial(startprob);

		// solve the starter problem to get the initial solution
		ierr = starttime->solve(u); CHKERRQ(ierr);
	}

	// Reset the KSP - could be advantageous for some types of algebraic solvers
	ierr = KSPDestroy(&ksp); CHKERRQ(ierr);
	destroyBlastedDataVec(bctx);
	ierr = KSPCreate(PETSC_COMM_WORLD, &ksp); CHKERRQ(ierr);
	if(mf_flg) {
		ierr = KSPSetOperators(ksp, A, M); 
		CHKERRQ(ierr);
	}
	else {
		ierr = KSPSetOperators(ksp, M, M); 
		CHKERRQ(ierr);
	}
	ierr = KSPSetFromOptions(ksp); CHKERRQ(ierr);
#ifdef USE_BLASTED
	// this will reset the timing
	bctx = newBlastedDataVec();
	if(opts.timesteptype == "IMPLICIT") {
		ierr = setup_blasted<NVARS>(ksp,u,startprob,bctx); CHKERRQ(ierr);
	}
#endif

	// setup nonlinear ODE solver for main solve - MUST be done AFTER KSPCreate
	if(opts.timesteptype == "IMPLICIT")
	{
		time = new SteadyBackwardEulerSolver<4>(prob, maintconf, ksp);
		std::cout << "\nSet up backward Euler temporal scheme for main solve.\n";
	}
	else
	{
		time = new SteadyForwardEulerSolver<4>(prob, u, maintconf);
		std::cout << "\nSet up explicit forward Euler temporal scheme for main solve.\n";
	}

	mfjac.set_spatial(prob);

	// Solve the main problem
	ierr = time->solve(u); CHKERRQ(ierr);

	std::cout << "***\n";

	delete starttime;
	delete time;
	ierr = KSPDestroy(&ksp); CHKERRQ(ierr);
	ierr = MatDestroy(&M); CHKERRQ(ierr);
	if(mf_flg) {
		ierr = MatDestroy(&A); 
		CHKERRQ(ierr);
	}

	// export output to VTU

	Array2d<a_real> scalars;
	Array2d<a_real> velocities;
	prob->postprocess_point(u, scalars, velocities);

	std::string scalarnames[] = {"density", "mach-number", "pressure", "temperature"};
	writeScalarsVectorToVtu_PointData(opts.vtu_output_file,
			m, scalars, scalarnames, velocities, "velocity");

	// export surface data like pressure coeff etc and volume data as plain text files

	MVector umat; umat.resize(m.gnelem(),NVARS);
	const PetscScalar *uarr;
	ierr = VecGetArrayRead(u, &uarr); CHKERRQ(ierr);
	for(a_int i = 0; i < m.gnelem(); i++)
		for(int j = 0; j < NVARS; j++)
			umat(i,j) = uarr[i*NVARS+j];
	ierr = VecRestoreArrayRead(u, &uarr);

	ierr = VecDestroy(&u); CHKERRQ(ierr);

	IdealGasPhysics phy(opts.gamma, opts.Minf, opts.Tinf, opts.Reinf, opts.Pr);
	FlowOutput out(&m, prob, &phy, opts.alpha);

	out.exportSurfaceData(umat, opts.lwalls, opts.lothers, opts.surfnameprefix);

	if(opts.vol_output_reqd == "YES")
		out.exportVolumeData(umat, opts.volnameprefix);

	delete prob;
	delete startprob;

#if 0
#ifdef USE_BLASTED
	// write out time taken by BLASTed preconditioner
	if(mpirank == 0) {
		const double linwtime = bctx.factorwalltime + bctx.applywalltime;
		const double linctime = bctx.factorcputime + bctx.applycputime;
		int numthreads = 1;
#ifdef _OPENMP
		numthreads = omp_get_max_threads();
#endif
		std::ofstream outf; outf.open(opts.logfile+"-precon.tlog", std::ofstream::app);
		// if the file is empty, write header
		outf.seekp(0, std::ios::end);
		if(outf.tellp() == 0) {
			outf << "# Time taken by preconditioning operations only:\n";
			outf << std::setw(10) << "# num-cells "
				<< std::setw(6) << "threads " << std::setw(10) << "wall-time "
				<< std::setw(10) << "cpu-time " << std::setw(10) << "avg-lin-iters "
				<< std::setw(10) << " time-steps\n";
		}

		// write current info
		outf << std::setw(10) << m.gnelem() << " "
			<< std::setw(6) << numthreads << " " << std::setw(10) << linwtime << " "
			<< std::setw(10) << linctime
			<< "\n";
		outf.close();
	}
#endif
#endif

	std::cout << '\n';
	ierr = PetscFinalize(); CHKERRQ(ierr);
	std::cout << "\n--------------- End --------------------- \n\n";
	return ierr;
}
