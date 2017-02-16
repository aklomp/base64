# Written in 2016-2017 by Henrik Steffen Gaßmann henrik@gassmann.onl
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
set(BASE64_VERSION 0.3.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(BASE64_GNU_C_COMPATIBLE 1)
endif()
if (CMAKE_C_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(BASE64_GNU_CXX_COMPATIBLE 1)
endif()


#######################################################################
# platform detection
include(TargetArch)
detect_target_architecture(_TARGET_ARCH)


########################################################################
# Compilation options
option(BASE64_INSTALL_TARGET "add an install target" ON)
option(BASE64_BUILD_TESTS "add test projects" OFF)

if (_TARGET_ARCH STREQUAL "x86" OR _TARGET_ARCH STREQUAL "x64")
    option(BASE64_WITH_SSSE3 "add SSSE 3 codepath" ON)
    option(BASE64_WITH_SSE41 "add SSE 4.1 codepath" ON)
    option(BASE64_WITH_SSE42 "add SSE 4.2 codepath" ON)
    option(BASE64_WITH_AVX "add AVX codepath" ON)
    option(BASE64_WITH_AVX2 "add AVX 2 codepath" ON)

elseif (_TARGET_ARCH STREQUAL "arm")
    option(BASE64_WITH_NEON32 "add NEON32 codepath" OFF)

elseif (_TARGET_ARCH STREQUAL "arm64")
    option(BASE64_WITH_NEON64 "add NEON64 codepath" ON)

endif()


###################################################################
# OpenMP
find_package(OpenMP)
if (OPENMP_FOUND)
    option(BASE64_USE_OpenMP "Utilize OpenMP to parallelize encoding and decoding." ON)
endif()


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
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -r")
endif()


########################################################################
# library project
add_library(base64 STATIC
    # library files
    lib/lib.c
    lib/codec_choose.c
    include/libbase64.h

    # codec implementations
    lib/arch/generic/codec.c

    lib/arch/ssse3/codec.c
    lib/arch/sse41/codec.c
    lib/arch/sse42/codec.c
    lib/arch/avx/codec.c
    lib/arch/avx2/codec.c

    lib/arch/neon32/codec.c
    lib/arch/neon64/codec.c
)

########################################################################
# SIMD settings
include(TargetSIMDInstructionSet)
define_SIMD_compile_flags()

if (_TARGET_ARCH STREQUAL "x86" OR _TARGET_ARCH STREQUAL "x64")
    macro(configure_codec _TYPE)
        if (BASE64_WITH_${_TYPE})
            string(TOLOWER "${_TYPE}" _DIR)
            set_source_files_properties("lib/arch/${_DIR}/codec.c" PROPERTIES
                COMPILE_FLAGS "${COMPILE_FLAGS_${_TYPE}}"
            )

            if (${ARGC} GREATER 1 AND MSVC)
                set_source_files_properties("lib/arch/${_DIR}/codec.c" PROPERTIES
                    COMPILE_DEFINITIONS ${ARGV1}
                )
            endif()
        endif()
    endmacro()

    configure_codec(SSSE3 __SSSE3__)
    configure_codec(SSE41 __SSSE4_1__)
    configure_codec(SSE42 __SSSE4_2__)
    configure_codec(AVX)
    configure_codec(AVX2)

elseif (_TARGET_ARCH STREQUAL "arm")
    set(BASE64_NEON32_CFLAGS "${COMPILE_FLAGS_NEON32}" CACHE STRING "the NEON32 compile flags (for 'lib/arch/neon32/codec.c')")
    mark_as_advanced(BASE64_NEON32_CFLAGS)

    if (BASE64_WITH_NEON32)
        set_source_files_properties("lib/arch/neon32/codec.c" PROPERTIES
            COMPILE_FLAGS "${BASE64_NEON32_CFLAGS} "
        )
    endif()

#elseif (_TARGET_ARCH STREQUAL "arm64" AND BASE64_WITH_NEON64)

endif()

target_compile_definitions(base64
    PRIVATE
        CMD_DEFINED_CONFIG
        HAVE_SSSE3=$<BOOL:${BASE64_WITH_SSSE3}>
        HAVE_SSE41=$<BOOL:${BASE64_WITH_SSE41}>
        HAVE_SSE42=$<BOOL:${BASE64_WITH_SSE42}>
        HAVE_AVX=$<BOOL:${BASE64_WITH_AVX}>
        HAVE_AVX2=$<BOOL:${BASE64_WITH_AVX2}>
        HAVE_NEON32=$<BOOL:${BASE64_WITH_NEON32}>
        HAVE_NEON64=$<BOOL:${BASE64_WITH_NEON64}>
)

########################################################################
# OpenMP Settings
if (BASE64_USE_OpenMP)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
endif()

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
