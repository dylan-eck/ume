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

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/spdlog)

find_package(Vulkan REQUIRED)

find_program(
  SLANGC_EXECUTABLE
  NAMES slangc
  HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin" NO_CACHE)

if(NOT SLANGC_EXECUTABLE)
  message(FATAL_ERROR "failed to locate slangc executable")
endif()

message(STATUS "Using slangc: ${SLANGC_EXECUTABLE}")

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/vk-bootstrap)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(B_PRODUCTION_MODE
      ON
      CACHE BOOL "" FORCE)
else()
  set(B_PRODUCTION_MODE
      OFF
      CACHE BOOL "" FORCE)
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/embed)

add_subdirectory(vendor/glm)

set(TINYGLTF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinygltf")

add_library(tinygltf STATIC "${TINYGLTF_DIR}/tiny_gltf_v3.c")
add_library(tinygltf::tinygltf ALIAS tinygltf)

target_include_directories(tinygltf SYSTEM PUBLIC "${TINYGLTF_DIR}")

set_target_properties(
  tinygltf
  PROPERTIES C_STANDARD 11
             C_STANDARD_REQUIRED ON
             C_EXTENSIONS OFF
             POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(tinygltf PUBLIC TINYGLTF3_ENABLE_FS)

set(WREN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/wren")
add_library(
  wren STATIC
  "${WREN_DIR}/src/vm/wren_compiler.c"
  "${WREN_DIR}/src/vm/wren_core.c"
  "${WREN_DIR}/src/vm/wren_debug.c"
  "${WREN_DIR}/src/vm/wren_primitive.c"
  "${WREN_DIR}/src/vm/wren_utils.c"
  "${WREN_DIR}/src/vm/wren_value.c"
  "${WREN_DIR}/src/vm/wren_vm.c"
  "${WREN_DIR}/src/optional/wren_opt_meta.c"
  "${WREN_DIR}/src/optional/wren_opt_random.c")

add_library(wren::wren ALIAS wren)

target_include_directories(wren PUBLIC "${WREN_DIR}/src/include")
target_include_directories(wren PRIVATE "${WREN_DIR}/src/optional")
target_include_directories(wren PRIVATE "${WREN_DIR}/src/vm")

set_target_properties(
  wren
  PROPERTIES C_STANDARD 99
             C_STANDARD_REQUIRED ON
             C_EXTENSIONS OFF
             POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(wren PRIVATE WREN_OPT_META=1 WREN_OPT_RANDOM=1)
