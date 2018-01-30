# ttcr

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](./01_LICENSE.txt)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.1162725.svg)](https://doi.org/10.5281/zenodo.1162725)

This repo contains C++ codes for raytracing on regular and unstructuted meshes.
Python and Matlab wrappers are provided as well.

## Stand alone programs

There are three programs that can be called from the command line:

- ttcr2d : raytracing on planar 2D meshes
- ttcr2ds : raytracing on undulated surfaces
- ttcr3d : raytracing in 3D

Each program is invoked with the same following options:
```
-p  Specify parameter file (mandatory)
-h  print short help message
-k  Save model in VTK format
-v  Verbose mode
-t  Measure time to build grid and perform raytracing
```

### Parameter file

The parameter file is used to specify the raytracing parameters.  It is a plain ascii file and each line has the following format:
```
value          # keyword,
```
The keywords are :

-  **basename** : each output file will start with this basename
-  **modelfile** : name of file holding grid definition, currently VTK and gmsh formats are accepted (see below)
-  **velfile** : velocity in physical domains defined in gmsh files
-  **slofile** : slowness of the cells defined in gmsh files
-  **srcfile** : name of file containing source location and t0
-  **rcvfile** : name of file containing receiver location
-  **secondary nodes** : number of secondary nodes for the shortest-path method (SPM)
-  **number of threads** : perform raytracing for multiple sources simultaneously using this number of threads
-  **inverse distance** : use inverse distance instead of linear interpolation for computing slowness at secondary nodes (SPM in 3D)
-  **metric order** : metric used to built sweeping ordering (FSM, see Qian et al. 2007) default is 2
-  **epsilon** : convergence criterion (FSM, see Qian et al. 2007) default is 1.e-15
-  **max number of iteration** : max number of sweeping iterations (FSM) default is 20
-  **saveGridTT** : save traveltime over whole grid, in ASCII file if 1 or in VTK format if 2.
-  **single precision** : work with float rather than double
-  **fast marching** : use fast marching method if value == 1 (implemented on 2D & 3D unstructured meshes only)
-  **fast sweeping**: use fast sweeping method if value == 1
-  **process reflectors** :
-  **saveRayPaths** :
-  **raypath high order** : compute traveltime gradient on unstructured meshes with high order least-squares (default is 0)
-  **fsm high order** : use 3rd order weighted essentially non-oscillatory (WENO) operator with fast sweeping in rectilinear grid if value == 1 (default is 0)

An example is shown below (note that keywords *must* be comprised between a hashtag and a comma):
```
ttcr2ds        # basename,
model2ds.msh   # modelfile,
model2ds.vel   # velfile,
src.dat        # srcfile,
rcv.dat        # rcvfile,
1              # number of threads,
1              # saveGridTT,
5              # secondary nodes,
1              # saveRayPaths,
```

### Model file

#### Rectilinear grids

Models can be defined in VTK (http://vtk.org) format or custom GRD format.

**VTK files**: These files must hold the slowness data, which can be defined either in terms of slowness or velocity, as cell data for grid with cells of constant slowness or as point data for slowness defined at grid nodes.

**GRD files**: For this format, the grid dimensions are given in a file with a .grd extension.  The format is the following, where values are given for x, y and z in that order (2D grids should have 0 cells in y):

Example `example.grd`
```
100 0 200 # number of cells,
1 1 1 # size of cells,
0 0 0 # origin of grid,
```

Slowness values should be provided separately in a `slofile`.  Slowness values can be assigned to nodes or cells, depending on the total number of slowness values given in the file: if the number is equal to the number of nodes, then the values are assigned to nodes and if the number is equal to the number of cells then the values are assigned to the cells.

** Important **

In 2D, the values are ordered for z incremented first, then x, while in 3D the order is x first, then y and finally z, i.e.

In 2D, ordering should correspond to coordinates
```
xmin      zmin
xmin      zmin+dz
xmin      zmin+2dz
  :        :
xmin      zmax
xmin+dx   zmin
xmin+dx   zmin+dz
  :        :
xmax      zmax
```
In 3D, ordering should correspond to coordinates
```
xmin      ymin     zmin
xmin+dx   ymin     zmin
xmin+2dx  ymin     zmin
  :        :        :
xmax      ymin     zmin
xmin      ymin+dy  zmin
xmin+dx   ymin+dy  zmin
xmin+2dx  ymin+dy  zmin
  :        :        :
xmax      ymin+dy  zmin
  :        :        :
xmax      ymax     zmin
xmin      ymin     zmin+dz
xmin+dx   ymin     zmin+dz
  :        :        :
xmax      ymin     zmin+dz
xmin      ymin+dy  zmin+dz
  :        :        :

etc
```

###### Note regarding the fast sweeping method on rectilinear grids

The 3D implementations require that the cells must be cubic .  Only the first value for the size of cell is used when building the grids.

#### Unstructured meshes

Models are defined in either VTK (http://www.vtk.org) or gmsh (http://geuz.org/gmsh/) files.

**VTK files**: As for rectilinear grids, these files must hold the slowness data, which can be defined either in terms of slowness or velocity (see routines in files grids.h and VTUReader.h for details).

**MSH files**: gmsh file format version 2.2 is supported.  This format does not allow storing cell attributes, so slowness data must be stored in other files.  There are two options: the first is to have a file holding the slowness values for each cell, in the same cell order than found in the msh file.  This type of file corresponds to the `slofile` found in the parameter file.  The other option is to define velocity values for physical entities (volumes in 3D or surfaces in 2D) found in msh files.  These data are given in `velfile`.  The following gives an example of a geometry file used by gmsh to generate the mesh, and the associated `velfile`.

Example `model2ds.geo`
```
lc = 25;

Point(1) = {    0,    0, 200, lc };
Point(2) = {    0, 1000, 250, lc };
Point(3) = { 1000, 1000, 225, lc };
Point(4) = { 1000,    0, 275, lc };
Point(5) = {    0,  300, 175, lc };
Point(6) = {    0,  600, 300, lc };
Point(7) = {  500, 1000, 150, lc };
Point(8) = { 1000,  700, 200, lc };
Point(9) = { 1000,  250, 200, lc };
Point(10) = { 500,    0, 350, lc };
Point(11) = { 500,  750, 150, lc };
Point(12) = { 600,  300, 375, lc };

Spline(1) = { 1, 5, 6 };
Spline(2) = { 2, 7, 3 };
Spline(3) = { 3, 8, 9 };
Spline(4) = { 4, 10, 1 };

Spline(5) = { 6, 11, 3 };
Line(6) = { 6, 2 };
Spline(7) = { 9, 12, 1 };
Line(8) = { 9, 4 };

Line Loop(1) = { 1, 5, 3, 7 };
Line Loop(2) = { 2, -5, 6 };
Line Loop(3) = { 8, 4, -7 };

Ruled Surface(1) = { 1 };
Ruled Surface(2) = { 2 };
Ruled Surface(3) = { 3 };

Physical Surface("S1") = {1};
Physical Surface("S2") = {2};
Physical Surface("S3") = {3};
```

Note that it is possible to impose the creation of given point nodes related to geometrical entities (handy for isolated source points), by defining such points as "physical points" in the geometry file.

Example `model2ds.vel`
```
"S1"    3000
"S2"    5000
"S3"    2000
```

###### Slowness defined at grid nodes for unstructured meshes

It is possible to define slowness at grid nodes rather than for mesh cells.  This improves accuracy for models where slowness gradients are present.  For VTK files, this is done by assigning slowness or velocity values at nodes.  For MSH files, this is done by using a `slofile` with slowness values for each mesh node in the order found in the msh file (`velfile` cannot be used in this case).

### Src and Rcv files

These are simple ascii file holding the coordinates of the source points and initial time (t0) at the points, and the coordinates of the receivers.  *Note that files used by ttcr2d should not have y coordinates.*  The following are two examples for ttcr3d.  

Example `src1.dat`
```
2
0 0 0 0
1 0 0 0
```

Example `rcv.dat`
```
10
 10 0 0
 20 0 0
 30 0 0
 40 0 0
 50 0 0
 60 0 0
 70 0 0
 80 0 0
 90 0 0
100 0 0
```

Note that one "source" can have many points in space.  When raytracing is needed for many sources, simply add the files in the parameter file, as shown below.  In that case, setting number of threads to 3 will tell the program to perform raytracing simultaneously for the three sources.  Note that number of threads should *not* be set to a number larger than the number of CPU cores of your machine.  If you have more sources that CPU cores, set number of threads to the number of cores, and raytracing will automatically be performed in "batches".
```
ttcr2ds        # basename,
model2ds.msh   # modelfile,
model2ds.vel   # velfile,
src1.dat       # srcfile,
src2.dat       # srcfile,
src3.dat       # srcfile,
rcv.dat        # rcvfile,
3              # number of threads,
1              # saveGridTT,
5              # secondary nodes,
1              # saveRayPaths,
```

### Examples

Look at the files in the examples directory for some samples.

### Compiling

The programs are coded in C++ and follow the C++11 standard.  You must have VTK (http://vtk.org) installed on your system, as well as the eigen3 (http://eigen.tuxfamily.org) and boost (http://www.boost.org) libraries.  These codes were compiled and tested on macs with the default compiler (clang).  They were also tested to some extent under linux with g++ version 4.8.

## Matlab wrappers

To compile the mexfiles, you will need:
- a C++ compiler that conforms to the C++11 standard
- to have the eigen3 library installed on your system (http://eigen.tuxfamily.org)
- as well as the boost library (http://www.boost.org)
- the source codes of the ttcr package


On my OS X machine, I use this command to compile from a terminal:
```
MATLAB=/Applications/MATLAB_R2014a.app

$MATLAB/bin/mex -O CXXFLAGS='$CXXFLAGS -std=c++11 -stdlib=libc++' LDFLAGS='$LDFLAGS -std=c++11 -stdlib=libc++' -largeArrayDims -v -I$HOME/src/ttcr/ttcr -I/opt/local/include -I/opt/local/include/eigen3 grid2duisp_mex.cpp
```

On a windows 8.1 machine with intel compiler installed, I could compile it from the matlab prompt with:
```
mex -v -O COMPFLAGS='$COMPFLAGS /Qstd=c++11' -largeArrayDims -I../ttcr -I'C:\libraries\boost_1_61_0' -I'C:\Program Files (x86)\Eigen\include' grid2duisp_mex.cpp
```
I got windows binaries of the eigen library from http://pointclouds.org/downloads/windows.html


Unfortunately, I cannot offer extensive support for compiling on other platforms, especially windows variants.


Please report bugs to bernard dot giroux at ete.inrs.ca




## References
```
@ARTICLE{giroux13,
  author = {Bernard Giroux and Beno\^{\i}t Larouche},
  title = {Task-parallel implementation of {3D} shortest path raytracing for
	geophysical applications},
  journal = {Computers & Geosciences},
  year = {2013},
  volume = {54},
  pages = {130--141},
  number = {0},
  doi = {10.1016/j.cageo.2012.12.005}
  url = {http://dx.doi.org/10.1016/j.cageo.2012.12.005}
}

@INPROCEEDINGS{giroux13b,
  author = {Bernard Giroux},
  title = {Shortest path raytracing on tetrahedral meshes},
  booktitle = {75$^{th}$ EAGE Conference \& Exhibition},
  year = {2013},
  address = {London},
  organization = {EAGE},
  doi = {10.3997/2214-4609.20130236}
  url = {http://dx.doi.org/10.3997/2214-4609.20130236}
}

@ARTICLE{lelievre11,
  author = {Leli\`evre, Peter G. and Farquharson, Colin G. and Hurich, Charles A.},
  title = {Computing first-arrival seismic traveltimes on unstructured 3-{D}
	tetrahedral grids using the Fast Marching Method},
  journal = {Geophysical Journal International},
  year = {2011},
  volume = {184},
  pages = {885-896},
  number = {2},
  doi = {10.1111/j.1365-246X.2010.04880.x}
  url = {http://dx.doi.org/10.1111/j.1365-246X.2010.04880.x}
}

@ARTICLE{qian07,
  author = {Qian, Jianliang and Zhang, Yong-Tao and Zhao, Hong-Kai},
  title = {Fast Sweeping Methods for Eikonal Equations on Triangular Meshes},
  journal = {SIAM Journal on Numerical Analysis},
  year = {2007},
  volume = {45},
  pages = {83--107},
  number = {1},
  doi = {10.1137/050627083},
  publisher = {Society for Industrial and Applied Mathematics},
  url = {http://www.jstor.org/stable/40232919}
}

@Article{zhang06,
  Title                    = {High Order Fast Sweeping Methods for Static {H}amilton–{J}acobi Equations},
  Author                   = {Yong-Tao Zhang and Hong-Kai Zhao and Jianliang Qian},
  Journal                  = {Journal of Scientific Computing},
  Year                     = {2006},
  Number                   = {1},
  Pages                    = {25--56},
  Volume                   = {29},
  DOI                      = {10.1007/s10915-005-9014-3},
  URL                      = {http://dx.doi.org/10.1007/s10915-005-9014-3}
}

@Article{zhao05,
  Title                    = {A Fast Sweeping Method for Eikonal Equations},
  Author                   = {Zhao, Hongkai},
  Journal                  = {Mathematics of Computation},
  Year                     = {2005},
  Month                    = apr,
  Number                   = {250},
  Pages                    = {603--627},
  Volume                   = {74},
  Publisher                = {American Mathematical Society},
  URL                      = {http://www.jstor.org/stable/4100081}
}
```
