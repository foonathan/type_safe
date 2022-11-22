# This function will do whatever is needed to a target to make the standard library module available
# This may include setting flags, adding files, ....
# It also checks the compiler
function(type_safe_add_std_module)

    cmake_parse_arguments(
        ARGS
        "VERBOSE"
        "TARGET"
        ""
        "${ARGN}")

    if(TYPE_SAFE_IMPORT_STD_MODULE)
        # Hopefully MS will provide a better/automatic/IJW way to use the standard library module, in other words "import std;" should just work
        if( MSVC )
            if(ARGS_VERBOSE)
                message(STATUS "Adding standard library for MSVC version ${MSVC_VERSION}")
            endif( )

            if( ${MSVC_VERSION}  LESS 1935)
                message(STATUS "Standard library is not available MSVC for version ${MSVC_VERSION}, minimum version is 1935 aka VS 17.5")
            endif()

            # We use a path relative to the compiler location which should be stable avoid VS version/Preview install location issues
            cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH  CompilerDir)
            target_sources(${ARGS_TARGET} PRIVATE "${CompilerDir}/../../../modules/std.ixx" )
        else()
            message(FATAL_ERROR "The standard library module is not support for ${CMAKE_CXX_COMPILER_ID} version ${CMAKE_CXX_COMPILER_VERSION}")
        endif()
    endif()

endfunction()