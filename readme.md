OpenCL-SPH
==========

An implementation of smoothed particle hydrodynamics using OpenCL.

This code is under development and may be subject to heavy revision at any time.

The nearest-neighbour search (NNS) algorithm is derived from the method used for [Fluids v3. by Rama Hoetzlein](https://github.com/rchoetzlein/fluids3).

Installation
------------

* Clone this repository

  ```shell
  git clone https://github.com/tauroid/opencl-sph
  ```

* `cmake .`

* `make`

You will need OpenCL headers and libraries.

This has only been tested on linux, with gcc.

Usage
-----

There are a number of Jupyter Notebooks in the [scripts](scripts) folder calling the python interface in various ways - see these for example usage.
