OpenCL-SPH
==========

An implementation of smoothed particle hydrodynamics using OpenCL.

This code is under development and may be subject to heavy revision at any time.

Installation
------------

* Clone this repository

  ```shell
  git clone https://github.com/tauroid/opencl-sph
  ```

* Edit the first line of [makefile](makefile) to point to your MATLAB installation

* `make`

You will need OpenCL headers and libraries.

This has only been tested on linux, with gcc.

Usage
-----

The current interface is through MATLAB - see [go.m](go.m) or [go_fluid.m](go_fluid.m).

These implement an elastic solid implementation based on [Becker et al. 2009](http://cg.informatik.uni-freiburg.de/publications/2009_NP_corotatedSPH.pdf) and a simple pressure gradient fluid respectively.

Using particle configurations other than a cube at the moment requires manual specification in the `position` field of psdata.
