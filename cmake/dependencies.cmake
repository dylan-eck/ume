function(ume_silence_vendor_warnings dir)
  get_property(
    targets
    DIRECTORY "${dir}"
    PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS targets)
    get_target_property(type ${target} TYPE)
    if(type MATCHES "^(STATIC|SHARED|MODULE|OBJECT)_LIBRARY$|^EXECUTABLE$")
      target_compile_options(${target} PRIVATE -w)
    endif()
  endforeach()

  get_property(
    subdirs
    DIRECTORY "${dir}"
    PROPERTY SUBDIRECTORIES)
  foreach(subdir IN LISTS subdirs)
    ume_silence_vendor_warnings("${subdir}")
  endforeach()
endfunction()

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

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL SYSTEM)
ume_silence_vendor_warnings(${CMAKE_SOURCE_DIR}/vendor/SDL)

add_library(glaze INTERFACE)
target_include_directories(glaze SYSTEM
                           INTERFACE ${CMAKE_SOURCE_DIR}/vendor/glaze/include)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/spdlog SYSTEM)
ume_silence_vendor_warnings(${CMAKE_SOURCE_DIR}/vendor/spdlog)

find_path(
  SLANG_INCLUDE_DIR slang.h
  HINTS "$ENV{VULKAN_SDK}/include"
  PATH_SUFFIXES slang)
find_library(
  SLANG_LIBRARY
  NAMES slang
  HINTS "$ENV{VULKAN_SDK}/lib")
if(NOT SLANG_INCLUDE_DIR OR NOT SLANG_LIBRARY)
  message(FATAL_ERROR "failed to locate slang library/headers")
endif()

add_library(slang::slang UNKNOWN IMPORTED)
set_target_properties(
  slang::slang PROPERTIES IMPORTED_LOCATION "${SLANG_LIBRARY}"
                          INTERFACE_INCLUDE_DIRECTORIES "${SLANG_INCLUDE_DIR}")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(B_PRODUCTION_MODE
      ON
      CACHE BOOL "" FORCE)
else()
  set(B_PRODUCTION_MODE
      OFF
      CACHE BOOL "" FORCE)
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/embed SYSTEM)
ume_silence_vendor_warnings(${CMAKE_SOURCE_DIR}/vendor/embed)

add_subdirectory(vendor/glm SYSTEM)
ume_silence_vendor_warnings(${CMAKE_SOURCE_DIR}/vendor/glm)

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

target_include_directories(wren SYSTEM PUBLIC "${WREN_DIR}/src/include")
target_include_directories(wren PRIVATE "${WREN_DIR}/src/optional")
target_include_directories(wren PRIVATE "${WREN_DIR}/src/vm")

set_target_properties(
  wren
  PROPERTIES C_STANDARD 99
             C_STANDARD_REQUIRED ON
             C_EXTENSIONS OFF
             POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(wren PRIVATE WREN_OPT_META=1 WREN_OPT_RANDOM=1)
target_compile_options(wren PRIVATE -w)
