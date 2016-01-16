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

include(TargetArch)
include(CheckSymbolExists)

########################################################################
# compiler flags definition
macro(define_SIMD_compile_flags)
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(COMPILE_FLAGS_SSSE3 "-mssse3")
        set(COMPILE_FLAGS_AVX2 "-mavx2")
    #elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC") <- sorry about that, but old cmake versions think "MSVC" is a variable and must be dereferenced :(
    elseif(MSVC)
        set(COMPILE_FLAGS_SSSE3 " ")
        set(COMPILE_FLAGS_AVX2 "/arch:AVX2")
    endif()
endmacro(define_SIMD_compile_flags)

########################################################################
# compiler feature detection (incomplete & currently unused)
function(detect_target_SIMD_instruction_set_SSSE3 OUTPUT_VARIABLE)
    define_SIMD_compile_flags()
    
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${COMPILE_FLAGS_SSSE3}")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        check_symbol_exists(__SSSE3__ "tmmintrin.h" DTCTN_VALUE)
    #elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    elseif(MSVC)
        # with msvc one would have to try to compile a program referencing
        # all used intrinsics. However I do know for sure that MSVC
        # supports SSSE3 since MSVC 14 / VS2008...
        if (MSVC_VERSION GREATER 1300)
            set(DTCTN_VALUE 1)
        else()
            set(DTCTN_VALUE 0)
        endif()
    endif()
    
    if (DTCTN_VALUE)
        set(${OUTPUT_VARIABLE} ${${OUTPUT_VARIABLE}} SSSE3-FOUND PARENT_SCOPE)
    else()
        set(${OUTPUT_VARIABLE} ${${OUTPUT_VARIABLE}} SSSE3-NOTFOUND PARENT_SCOPE)
    endif()
endfunction(detect_target_SIMD_instruction_set_SSSE3)

function(detect_target_SIMD_instruction_set_AVX2 OUTPUT_VARIABLE)
    define_SIMD_compile_flags()
    
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${COMPILE_FLAGS_AVX2}")
    check_symbol_exists(__AVX2__ "immintrin.h" DTCTN_VALUE)
    
    if (DTCTN_VALUE)
        set(${OUTPUT_VARIABLE} ${${OUTPUT_VARIABLE}} AVX2-FOUND PARENT_SCOPE)
    else()
        set(${OUTPUT_VARIABLE} ${${OUTPUT_VARIABLE}} AVX2-NOTFOUND PARENT_SCOPE)
    endif()
endfunction(detect_target_SIMD_instruction_set_AVX2)

function(detect_target_SIMD_instruction_set_NEON OUTPUT_VARIABLE)
    
endfunction(detect_target_SIMD_instruction_set_NEON)

function(detect_target_SIMD_instruction_set OUTPUT_VARIABLE)
    target_architecture(_TARGET_ARCH)
    
    if (APPLE AND CMAKE_OSX_ARCHITECTURES
        OR _TARGET_ARCH STREQUAL "i386" 
        OR _TARGET_ARCH STREQUAL "x86_64")
        detect_target_SIMD_instruction_set_SSSE3(_TEMP_OUTPUT)
        detect_target_SIMD_instruction_set_AVX2(_TEMP_OUTPUT)
    elseif(_TARGET_ARCH MATCHES arm)
        # add neon detection
    endif()
    set(${OUTPUT_VARIABLE} ${_TEMP_OUTPUT} PARENT_SCOPE)
endfunction(detect_target_SIMD_instruction_set)
