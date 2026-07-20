set(SDL_SHARED
    OFF
    CACHE BOOL "" FORCE)
set(SDL_STATIC
    ON
    CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY
    OFF
    CACHE BOOL "" FORCE)
set(SDL_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(SDL_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
set(SDL_AUDIO
    OFF
    CACHE BOOL "" FORCE)
set(SDL_CAMERA
    OFF
    CACHE BOOL "" FORCE)
set(SDL_SENSOR
    OFF
    CACHE BOOL "" FORCE)
set(SDL_RENDER
    OFF
    CACHE BOOL "" FORCE)
set(SDL_GPU
    OFF
    CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL)

add_library(glaze INTERFACE)
target_include_directories(glaze
                           INTERFACE ${CMAKE_SOURCE_DIR}/vendor/glaze/include)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/sol2)

# Lua
add_library(
  lua STATIC
  ${CMAKE_SOURCE_DIR}/vendor/lua/lapi.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lauxlib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lbaselib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lcode.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lcorolib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lctype.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/ldblib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/ldebug.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/ldo.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/ldump.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lfunc.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lgc.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/linit.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/liolib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/llex.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lmathlib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lmem.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/loadlib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lobject.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lopcodes.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/loslib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lparser.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lstate.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lstring.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lstrlib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/ltable.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/ltablib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/ltm.c
  # ${CMAKE_SOURCE_DIR}/vendor/lua/lua.c
  # ${CMAKE_SOURCE_DIR}/vendor/lua/luac.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lundump.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lutf8lib.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lvm.c
  ${CMAKE_SOURCE_DIR}/vendor/lua/lzio.c)

target_include_directories(lua PUBLIC ${CMAKE_SOURCE_DIR}/vendor/lua)
