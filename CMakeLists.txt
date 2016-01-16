# Written in 2016 by Henrik Steffen Gaﬂmann henrik@gassmann.onl
#
# To the extent possible under law, the author(s) have dedicated all
# copyright and related and neighboring rights to this software to the
# public domain worldwide. This software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain Dedication
# along with this software. If not, see
#
#     http://creativecommons.org/publicdomain/zero/1.0/
#
########################################################################
cmake_minimum_required(VERSION 2.8.12)
project(base64)
set(BASE64_VERSION 0.2.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(BASE64_GNU_C_COMPATIBLE 1)
endif()
if (CMAKE_C_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(BASE64_GNU_CXX_COMPATIBLE 1)
endif()

# set BASE64_INCLUDED to 1 if you include this project from another
# cmake project (this is handy in combination with git submodules :)
if (NOT BASE64_INCLUDED)
    option(BASE64_INSTALL_TARGET "add an install target" ON)
    option(BASE64_BUILD_TESTS "add test projects" OFF)
    
    include(TargetSIMDInstructionSet)
    option(BASE64_WITH_SSSE3 "add SSSE3 codepath" OFF)
    option(BASE64_WITH_AVX2 "add AVX2 codepath" OFF)
    
    #list(APPEND BASE64_NEON_OPTIONS NEON32)
    #list(APPEND BASE64_NEON_OPTIONS NEON32-DEFAULTED)
    #list(APPEND BASE64_NEON_OPTIONS NEON64)
    #if (BASE64_NEON_OPTIONS)
    #set(BASE64_WITH_NEON "none" CACHE STRING "which NEON codepath should be choosen")
    #    list(APPEND BASE64_NEON_OPTIONS none)
    #    set_property(CACHE BASE64_WITH_NEON PROPERTY STRINGS ${BASE64_NEON_OPTIONS})
    #    if (NOT ";${BASE64_WITH_NEON};" MATCHES ";${BASE64_NEON_OPTIONS};")
    #        message(FATAL_ERROR "invalid neon option selected ${BASE64_WITH_NEON}")
    #endif()
    
    ####################################################################
    # platform/compiler specific configuration
    if(MSVC)
        # Force to always compile with W4
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        if(${CMAKE_C_FLAGS} MATCHES "/W[0-4]")
            string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        else()
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
        endif()
    endif()
    if (BASE64_GNU_C_COMPATIBLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -Wpedantic")
    endif()
endif()

add_library(base64 STATIC
    lib/lib.c
    lib/codec_avx2.c
    lib/codec_choose.c
    lib/codec_neon32.c
    lib/codec_neon64.c
    lib/codec_plain.c
    lib/codec_ssse3.c
    
    include/libbase64.h
)

########################################################################
# SIMD settings
define_SIMD_compile_flags()
if (BASE64_WITH_SSSE3)
    set_source_files_properties(lib/codec_ssse3.c PROPERTIES
        COMPILE_FLAGS ${COMPILE_FLAGS_SSSE3}
    )
    if (MSVC)
        # if SSSE3 is available it is always enabled by default,
        # but MSVC _never_ defines __SSSE3__ 
        set_source_files_properties(lib/codec_ssse3.c PROPERTIES
            COMPILE_DEFINITIONS __SSSE3__
        )
    endif()
    set(HAVE_SSSE3 1)
else()
    set(HAVE_SSSE3 0)
endif()
if (BASE64_WITH_AVX2)
    set_source_files_properties(lib/codec_avx2.c PROPERTIES
        COMPILE_FLAGS ${COMPILE_FLAGS_AVX2}
    )
    set(HAVE_AVX2 1)
else()
    set(HAVE_AVX2 0)
endif()
# this needs much more love...
set(HAVE_NEON32 0)
set(HAVE_NEON64 0)
if (BASE64_WITH_NEON MATCHES NEON32)
    if(BASE64_WITH_NEON MATCHES NEON32-DEFAULTED)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILE_FLAGS_NEON32}")
    else()
        set_source_files_properties(lib/codec_neon32.c PROPERTIES
            COMPILE_FLAGS ${COMPILE_FLAGS_NEON32}
        )
    endif()
    set(HAVE_NEON32 1)
elseif (BASE64_WITH_NEON STREQUAL NEON64)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILE_FLAGS_NEON64}")
    set(HAVE_NEON64 1)
endif()


target_compile_definitions(base64
    PRIVATE
        CMD_DEFINED_CONFIG
        HAVE_SSSE3=${HAVE_SSSE3}
        HAVE_AVX2=${HAVE_AVX2}
        HAVE_NEON32=${HAVE_NEON32}
        HAVE_NEON64=${HAVE_NEON64}
)

########################################################################
if (BASE64_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

########################################################################
# install target
if (BASE64_INSTALL_TARGET)
    install(TARGETS base64 EXPORT base64-targets
        RUNTIME DESTINATION bin/$<CONFIG>
        LIBRARY DESTINATION lib/$<CONFIG>
        ARCHIVE DESTINATION lib/$<CONFIG>
        INCLUDES DESTINATION include
    )
    install(FILES include/libbase64.h DESTINATION include)

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/base64-config-version.cmake"
        VERSION ${BASE64_VERSION}
        COMPATIBILITY ExactVersion
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/base64-config-version.cmake" DESTINATION cmake)

    configure_file(cmake/base64-config.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/base64-config.cmake"
        COPYONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/base64-config.cmake" DESTINATION cmake)

    install(EXPORT base64-targets DESTINATION cmake)
endif()
