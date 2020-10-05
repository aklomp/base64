set(PROJECT_NAME base64)
project(${PROJECT_NAME})
add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE
    lib/arch/avx2/codec.c
    lib/arch/generic/codec.c
    lib/arch/neon32/codec.c
    lib/arch/neon64/codec.c
    lib/arch/ssse3/codec.c
    lib/arch/sse41/codec.c
    lib/arch/sse42/codec.c
    lib/arch/avx/codec.c
    lib/lib.c
    lib/codec_choose.c
    lib/tables/tables.c
)

target_include_directories(${PROJECT_NAME} PUBLIC .)
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-mavx2>
)
