#!/bin/bash

module load intel intel-opencl-sdk broadwell mvapich forge

cmake -DCMAKE_INSTALL_PREFIX=../install .. -DOpenCL_LIBRARY=/pawsey/opencl-sdk/7.0.0/opencl/SDK/lib64/libOpenCL.so -DCMAKE_EXE_LINKER_FLAGS="-L${HOME} -lmap-sampler-pmpi -lmap-sampler -Wl,--eh-frame-hdr -Wl,-rpath=${HOME}" -DCMAKE_BUILD_TYPE=debug
