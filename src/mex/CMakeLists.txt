find_package(Matlab COMPONENTS MX_LIBRARY)

string(REGEX REPLACE "([^;]+)" "../\\1" SPH_SOURCES_MEX "${SPH_SOURCES}")

if (Matlab_FOUND)
    include_directories(${Matlab_INCLUDE_DIRS})
    
    add_library(SPH_mex SHARED ${SPH_SOURCES_MEX})
    set_property(TARGET SPH_mex PROPERTY POSITION_INDEPENDENT_CODE ON)
    if (WIN32)
        set_property(TARGET SPH_mex PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif (WIN32)
    target_compile_definitions(SPH_mex PUBLIC MATLAB_MEX_FILE)
    target_link_libraries(SPH_mex ${SPH_LIBS} ${Matlab_MEX_LIBRARY} ${Matlab_MX_LIBRARY})

    if (MSVC)
        set_target_properties(SPH_mex PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/mex")
    endif (MSVC)

    set(MEX_FILES
        callkernel
        computebins
        initsph
        querystate
        setstate
        simstep
        simstep_fluid
        syncalldevicetohost
        syncallhosttodevice
    )

    foreach (MEX_FILE IN LISTS MEX_FILES)
        matlab_add_mex(NAME ${MEX_FILE} SRC ${MEX_FILE}.c LINK_TO SPH_mex)
    endforeach (MEX_FILE)

    if (MSVC)
        set_target_properties(${MEX_FILES} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/mex")
    endif (MSVC)
endif(Matlab_FOUND)
