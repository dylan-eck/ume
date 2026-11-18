set(FASTNOISE2_TOOLS
    OFF
    CACHE BOOL "" FORCE)
set(FASTNOISE2_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(FASTNOISE2_UTILITY
    OFF
    CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Not SYSTEM: Apple clang searches /usr/local/include before -isystem paths, so
# a stray installed FastNoise2/FastSIMD there would shadow the vendored headers.
add_subdirectory(vendor/FastNoise2)
target_compile_options(FastNoise PRIVATE -w)
