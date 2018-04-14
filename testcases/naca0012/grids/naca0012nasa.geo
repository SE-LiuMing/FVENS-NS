// params
refine = 2;
// ---

radiusFF = 20;

meshSizeWing = 0.2/refine;
meshSizeLead = meshSizeWing/5.0;
meshSizeTrail = meshSizeWing/10.0;
meshSizeFF = radiusFF*meshSizeWing;

nsplinepoints = 20*refine;

// Shape of airfoil
Macro topsurface
y = 0.594689181*(0.298222773*Sqrt(x) - 0.127125232*x - 0.357907906*x^2 + 0.291984971*x^3 - 0.105174606*x^4);
Return
Macro botsurface
y = -0.594689181*(0.298222773*Sqrt(x) - 0.127125232*x - 0.357907906*x^2 + 0.291984971*x^3 - 0.105174606*x^4);
Return

// Airfoil points and splines

tsplinePoints[0] = 0;
Point(0) = {1,0,0,meshSizeTrail};

// assume range is inclusive
For i In {1:nsplinepoints-2}
	x = (1.0 - i/(nsplinepoints-1.0))^2;
	Call botsurface;
	tsplinePoints[i] = newp;
	//If(nsplinepoints-2-i < 2*refine)
		//Point(tsplinePoints[i]) = {x,y,0,meshSizeLead}; 
	//Else
		Point(tsplinePoints[i]) = {x,y,0,meshSizeWing}; 
	//EndIf
EndFor
tsplinePoints[nsplinepoints-1] = newp;
Point(tsplinePoints[nsplinepoints-1]) = {0,0,0,meshSizeLead}; 

bsplinePoints[0] = tsplinePoints[nsplinepoints-1];
For i In {1:nsplinepoints-2}
	x = (i/(nsplinepoints-1.0))^2;
	Call topsurface;
	bsplinePoints[i] = newp;
	//If(i < 2*refine)
		//Point(bsplinePoints[i]) = {x,y,0,meshSizeLead};
	//Else
		Point(bsplinePoints[i]) = {x,y,0,meshSizeWing};
	//EndIf
EndFor

bsplinePoints[nsplinepoints-1] = 0;

Spline(1) = tsplinePoints[];
Spline(2) = bsplinePoints[];

Point(10000) = {0,0,0, meshSizeLead};
Point(10001) = {radiusFF,0,0,meshSizeFF/1.5};
Point(10002) = {0,radiusFF,0,meshSizeFF};
Point(10003) = {-radiusFF,0,0,meshSizeFF};
Point(10004) = {0,-radiusFF,0,meshSizeFF};

//Point(20000) = {1.5,-0.5,0, meshSizeTrail};
//Point(20001) = {1.5,0.5,0, meshSizeTrail};

Circle(5) = {10001,10000,10002};
Circle(6) = {10002,10000,10003};
Circle(7) = {10003,10000,10004};
Circle(8) = {10004,10000,10001};

Line Loop(6) = {5,6,7,8};
Line Loop(7) = {1,2};
Plane Surface(10) = {6,7};
Physical Surface(1) = {10};
Physical Line(2) = {1,2};
Physical Line(4) = {5,6,7,8};

Recombine Surface(10);
//Mesh.SubdivisionAlgorithm=1;	// ensures all quads, I think

//Field[1] = Distance;
//Field[1].NodesList = {0};
//Background Field = 1;

Mesh.CharacteristicLengthFromCurvature = 1;
//Mesh.Algorithm=6;		// Frontal

Color Black{ Surface{10}; }
