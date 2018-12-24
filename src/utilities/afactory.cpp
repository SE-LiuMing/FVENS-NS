/** \file afactory.cpp
 * \brief Various factories for generating simulation-related objects
 * \author Aditya Kashi
 * \date 2017 October
 *
 * This file is part of FVENS.
 *   FVENS is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   FVENS is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with FVENS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "afactory.hpp"

#include "spatial/limitedlinearreconstruction.hpp"
#include "spatial/musclreconstruction.hpp"
#include "mpiutils.hpp"

namespace fvens {

template <typename scalar>
InviscidFlux<scalar>* create_mutable_inviscidflux(
		const std::string& type, 
		const IdealGasPhysics<scalar> *const p) 
{
	const int mpirank = get_mpi_rank(MPI_COMM_WORLD);
	InviscidFlux<scalar> *inviflux = nullptr;

	if(type == "VANLEER") {
		inviflux = new VanLeerFlux<scalar>(p);
		if(mpirank == 0)
			std::cout << " InviscidFluxFactory: Using Van Leer fluxes." << std::endl;
	}
	else if(type == "ROE")
	{
		inviflux = new RoeFlux<scalar>(p);
		if(mpirank == 0)
			std::cout << " InviscidFluxFactory: Using Roe fluxes." << std::endl;
	}
	else if(type == "HLL")
	{
		inviflux = new HLLFlux<scalar>(p);
		if(mpirank == 0)
			std::cout << " InviscidFluxFactory: Using HLL fluxes." << std::endl;
	}
	else if(type == "HLLC")
	{
		inviflux = new HLLCFlux<scalar>(p);
		if(mpirank == 0)
			std::cout << " InviscidFluxFactory: Using HLLC fluxes." << std::endl;
	}
	else if(type == "LLF")
	{
		inviflux = new LocalLaxFriedrichsFlux<scalar>(p);
		if(mpirank == 0)
			std::cout << " InviscidFluxFactory: Using LLF fluxes." << std::endl;
	}
	else if(type == "AUSM")
	{
		inviflux = new AUSMFlux<scalar>(p);
		if(mpirank == 0)
			std::cout << " InviscidFluxFactory: Using AUSM fluxes." << std::endl;
	}
	else if(type == "AUSMPLUS")
	{
		inviflux = new AUSMPlusFlux<scalar>(p);
		if(mpirank == 0)
			std::cout << " InviscidFluxFactory: Using AUSM+ fluxes." << std::endl;
	}
	else
		if(mpirank == 0)
			std::cout << " InviscidFluxFactory: ! Flux scheme not available!" << std::endl;

	return inviflux;
}

template <typename scalar>
const InviscidFlux<scalar>* create_const_inviscidflux(
		const std::string& type,
		const IdealGasPhysics<scalar> *const p) 
{
	return const_cast<const InviscidFlux<scalar>*>(create_mutable_inviscidflux(type, p));
}

// instantiations
template InviscidFlux<a_real>* create_mutable_inviscidflux(
		const std::string& type, 
		const IdealGasPhysics<a_real> *const p);
template const InviscidFlux<a_real>* create_const_inviscidflux(
		const std::string& type, 
		const IdealGasPhysics<a_real> *const p);

template <typename scalar, int nvars>
GradientScheme<scalar,nvars>* create_mutable_gradientscheme(const std::string& type, 
                                                            const UMesh2dh<scalar> *const m,
                                                            const scalar *const rc,
                                                            const scalar *const rcbp)
{
	const int mpirank = get_mpi_rank(MPI_COMM_WORLD);
	GradientScheme<scalar,nvars> * gradcomp = nullptr;

	if(type == "LEASTSQUARES")
	{
		gradcomp = new WeightedLeastSquaresGradients<scalar,nvars>(m, rc, rcbp);
		if(mpirank == 0)
			std::cout << " GradientSchemeFactory: Weighted least-squares gradients will be used.\n";
	}
	else if(type == "GREENGAUSS")
	{
		gradcomp = new GreenGaussGradients<scalar,nvars>(m, rc, rcbp);
		if(mpirank == 0)
			std::cout << " GradientSchemeFactory: Green-Gauss gradients will be used.\n";
	}
	else {
		gradcomp = new ZeroGradients<scalar,nvars>(m, rc, rcbp);
		if(mpirank == 0)
			std::cout << " GradientSchemeFactory: No gradient computation.\n";
	}

	return gradcomp;
}

template <typename scalar, int nvars>
const GradientScheme<scalar,nvars>* create_const_gradientscheme(const std::string& type, 
                                                                const UMesh2dh<scalar> *const m,
                                                                const scalar *const rc,
                                                                const scalar *const rcbp)
{
	return create_mutable_gradientscheme<scalar,nvars>(type, m, rc, rcbp);
}

// template instantiations
template GradientScheme<a_real,NVARS>* create_mutable_gradientscheme<a_real,NVARS>(
		const std::string& type, 
		const UMesh2dh<a_real> *const m,
		const a_real *const rc,
		const a_real *const rcbp);

template const GradientScheme<a_real,NVARS>* create_const_gradientscheme<a_real,NVARS>(
		const std::string& type, 
		const UMesh2dh<a_real> *const m,
		const a_real *const rc,
		const a_real *const rcbp);

template GradientScheme<a_real,1>* create_mutable_gradientscheme<a_real,1>(
		const std::string& type, 
		const UMesh2dh<a_real> *const m,
		const a_real *const rc,
		const a_real *const rcbp);

template const GradientScheme<a_real,1>* create_const_gradientscheme<a_real,1>(
		const std::string& type, 
		const UMesh2dh<a_real> *const m,
		const a_real *const rc,
		const a_real *const rcbp);


template <typename scalar, int nvars>
SolutionReconstruction<scalar,nvars>* create_mutable_reconstruction(const std::string& type,
                                                                    const UMesh2dh<scalar> *const m,
                                                                    const scalar *const rc,
                                                                    const scalar *const rcbp,
                                                                    const amat::Array2d<scalar>& gr,
                                                                    const a_real param)
{
	const int mpirank = get_mpi_rank(MPI_COMM_WORLD);
	SolutionReconstruction<scalar,nvars> * reconst = nullptr;

	if(type == "NONE")
	{
		reconst = new LinearUnlimitedReconstruction<scalar,nvars>(m, rc, rcbp, gr);
		if(mpirank == 0)
			std::cout << " ReconstructionFactory: Unlimited linear reconstruction selected.\n";
	}
	else if(type == "WENO")
	{
		reconst = new WENOReconstruction<scalar,nvars>(m, rc, rcbp, gr, param);
		if(mpirank == 0)
			std::cout << " ReconstructionFactory: WENO reconstruction selected.\n";
	}
	else if(type == "VANALBADA")
	{
		reconst = new MUSCLVanAlbada<scalar,nvars>(m, rc, rcbp, gr);
		if(mpirank == 0)
			std::cout << " ReconstructionFactory: Van Albada MUSCL reconstruction selected.\n";
	}
	else if(type == "BARTHJESPERSEN")
	{
		reconst = new BarthJespersenLimiter<scalar,nvars>(m, rc, rcbp, gr);
		if(mpirank == 0)
			std::cout << " ReconstructionFactory: Barth-Jespersen linear reconstruction selected.\n";
	}
	else if(type == "VENKATAKRISHNAN")
	{
		reconst = new VenkatakrishnanLimiter<scalar,nvars>(m, rc, rcbp, gr, param);
		if(mpirank == 0)
			std::cout << " ReconstructionFactory: Venkatakrishnan linear reconstruction selected.\n";
	}
	else {
		if(mpirank == 0)
			std::cout << " !ReconstructionFactory: Invalid reconstruction!!\n";
	}

	return reconst;
}

template <typename scalar, int nvars>
const SolutionReconstruction<scalar,nvars>*
create_const_reconstruction(const std::string& type, const UMesh2dh<scalar> *const m,
                            const scalar *const rc,
                            const scalar *const rcbp,
                            const amat::Array2d<scalar>& gr,
                            const a_real param)
{
	return create_mutable_reconstruction<scalar,nvars>(type, m, rc, rcbp, gr, param);
}

// template instantiations
template SolutionReconstruction<a_real,NVARS>*
create_mutable_reconstruction(const std::string& type,
                              const UMesh2dh<a_real> *const m, const a_real *const rc,
                              const a_real *const rcbp,
                              const amat::Array2d<a_real>& gr, const a_real param);
template const SolutionReconstruction<a_real,NVARS>*
create_const_reconstruction(const std::string& type,
                            const UMesh2dh<a_real> *const m, const a_real *const rc,
                            const a_real *const rcbp,
                            const amat::Array2d<a_real>& gr, const a_real param);

template SolutionReconstruction<a_real,1>*
create_mutable_reconstruction(const std::string& type,
                              const UMesh2dh<a_real> *const m, const a_real *const rc,
                              const a_real *const rcbp,
                              const amat::Array2d<a_real>& gr, const a_real param);
template const SolutionReconstruction<a_real,1>*
create_const_reconstruction(const std::string& type,
                            const UMesh2dh<a_real> *const m, const a_real *const rc,
                            const a_real *const rcbp,
                            const amat::Array2d<a_real>& gr, const a_real param);


template <typename scalar>
FlowFV_base<scalar>* create_mutable_flowSpatialDiscretization(
	const UMesh2dh<scalar> *const m,
	const FlowPhysicsConfig& pconf,
	const FlowNumericsConfig& nconf)
{
	if(nconf.order2)
		if(pconf.const_visc)
			return new FlowFV<scalar,true,true>(m, pconf, nconf);
		else
			return new FlowFV<scalar,true,false>(m, pconf, nconf);
	else
		if(pconf.const_visc)
			return new FlowFV<scalar,false,true>(m, pconf, nconf);
		else
			return new FlowFV<scalar,false,false>(m, pconf, nconf);
}

template <typename scalar>
const FlowFV_base<scalar>* create_const_flowSpatialDiscretization(
	const UMesh2dh<scalar> *const m,
	const FlowPhysicsConfig& pconf,
	const FlowNumericsConfig& nconf)
{
	return create_mutable_flowSpatialDiscretization<scalar>(m, pconf, nconf);
}

template
const FlowFV_base<a_real>*
create_const_flowSpatialDiscretization<a_real>(const UMesh2dh<a_real> *const m,
                                               const FlowPhysicsConfig& pconf,
                                               const FlowNumericsConfig& nconf);

template <int nvars>
const NonlinearUpdate<nvars>* create_const_nonlinearUpdateScheme(const FlowParserOptions& opts)
{
	const IdealGasPhysics<a_real> phy(opts.gamma, opts.Minf, opts.Tinf, opts.Reinf, opts.Pr);
	if(opts.nl_update_scheme == "FULL")
		return new FullUpdate<nvars>();
	else if(opts.nl_update_scheme == "ROBUST_FLOW")
		return new FlowSimpleUpdate<nvars>(phy, opts.min_nl_update);
	else
		throw std::invalid_argument("Unsupported nonlinear update scheme!");
}

template
const NonlinearUpdate<1>* create_const_nonlinearUpdateScheme(const FlowParserOptions& opts);
template
const NonlinearUpdate<NDIM+2>* create_const_nonlinearUpdateScheme(const FlowParserOptions& opts);

}
