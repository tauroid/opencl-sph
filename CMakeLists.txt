cmake_minimum_required(VERSION 3.9)
project(opencl_sph)

file(GLOB kernels kernels/*.cl)
file(GLOB 3rdparty src/3rdparty/*.h src/3rdparty/*.c)
file(GLOB opencl src/opencl/*.h src/opencl/*.c)
file(GLOB srcs src/*.h src/*.c)

find_package(OpenCL REQUIRED)

link_directories(${PCL_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")

add_library(sph_libs STATIC ${kernels} ${3rdparty} ${opencl})

add_executable(opencl_sph  ${srcs} )

target_link_libraries(opencl_sph sph_libs OpenCL m)