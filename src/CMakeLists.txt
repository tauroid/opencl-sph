project(OpenCL_SPH)

set(SPH_SOURCES
    build_psdata.c
    config.c
    note.c
    particle_system.c
    stringly.c
    3rdparty/whereami.c
    opencl/clerror.c
    opencl/particle_system_host.c
    opencl/platforminfo.c
)

find_package(OpenCL 1.2)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SPH_LIBS ${OpenCL_LIBRARIES})

if (NOT WIN32)
    set(SPH_LIBS ${SPH_LIBS} m)
endif(NOT WIN32)

include_directories(${OpenCL_INCLUDE_DIRS})

add_library(SPH SHARED ${SPH_SOURCES})
set_property(TARGET SPH PROPERTY POSITION_INDEPENDENT_CODE ON)
if (WIN32)
    set_property(TARGET SPH PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif(WIN32)
target_link_libraries(SPH ${SPH_LIBS})

add_executable(testopencl testopencl.c)
target_link_libraries(testopencl SPH)

if (MSVC)
    set_target_properties(SPH testopencl PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
                                                    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
endif (MSVC)

add_subdirectory(mex)
