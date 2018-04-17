cmake_minimum_required(VERSION 3.2)
project(opencl_sph)

set(CMAKE_C_STANDARD 11)

find_package(OpenCL 1.2 REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})
link_directories(${OpenCL_LIBRARY})

add_definitions(-DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_definitions(-DOPENCL_SPH_KERNELS_ROOT="${CMAKE_INSTALL_PREFIX}/share/opencl_sph/kernels/")
if(OPENCL_SPH_REAL_TYPE)
	add_definitions(-DOPENCL_SPH_REAL_TYPE=${OPENCL_SPH_REAL_TYPE})
endif(OPENCL_SPH_REAL_TYPE)

add_subdirectory(kernels)
add_subdirectory(src)
#SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -std=c99" )
