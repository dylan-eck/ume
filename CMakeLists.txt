cmake_minimum_required(VERSION 3.25)

project(
  ume
  VERSION 0.1.0
  LANGUAGES C CXX)

if(NOT APPLE)
  message(FATAL_ERROR "ume requires macOS (Metal)")
endif()

set(UME_COMPILER_LAUNCHER
    "auto"
    CACHE STRING
          "Compiler cache: auto, ccache, sccache, none, or a program name")
set_property(CACHE UME_COMPILER_LAUNCHER PROPERTY STRINGS auto sccache ccache
                                                  none)

if(UME_COMPILER_LAUNCHER STREQUAL "none")
  set(UME_LAUNCHER_EXECUTABLE "")
elseif(UME_COMPILER_LAUNCHER STREQUAL "auto")
  find_program(UME_LAUNCHER_EXECUTABLE NAMES ccache sccache)
else()
  find_program(UME_LAUNCHER_EXECUTABLE NAMES "${UME_COMPILER_LAUNCHER}"
                                             REQUIRED)
endif()

if(UME_LAUNCHER_EXECUTABLE)
  message(STATUS "ume: comR_EXECUTABLE}")
  foreach(lang IN ITEMS C CXX OBJC OBJCXX)
    set(CMAKE_${lang}_COMPILER_LAUNCHER "${UME_LAUNCHER_EXECUTABLE}")
  endforeach()
else()
  message(STATUS "ume: no compiler cache found, building without one")
endif()

include(cmake/dependencies.cmake)

add_library(ume_config INTERFACE)
target_compile_features(ume_config INTERFACE cxx_std_23)
target_compile_definitions(
  ume_config
  INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_EXPLICIT_CTOR
            GLM_ENABLE_EXPERIMENTAL $<$<CONFIG:Debug>:UME_ENABLE_LOGGING>)
target_link_libraries(ume_config INTERFACE glm::glm)

include(cmake/UmePlugin.cmake)

add_library(ume_plugin_api_c_check STATIC src/ume/plugin/plugin_api.c)
target_link_libraries(ume_plugin_api_c_check PRIVATE ume_plugin_api)
set_target_properties(
  ume_plugin_api_c_check
  PROPERTIES C_STANDARD 11
             C_STANDARD_REQUIRED ON
             C_EXTENSIONS OFF)
target_compile_options(
  ume_plugin_api_c_check
  PRIVATE $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic
          -Werror> $<$<C_COMPILER_ID:MSVC>:/W4 /WX>)

add_library(
  ume_engine STATIC
  src/ume/core/logger.cpp
  src/ume/core/engine.cpp
  src/ume/core/dynamic_library.cpp
  src/ume/platform/window.cpp
  src/ume/platform/input.cpp
  src/ume/renderer/renderer.cpp
  src/ume/renderer/shader_compiler.cpp
  src/ume/scripting/script_engine.cpp)
target_compile_options(
  ume_engine PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra
                     -Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4>)
target_include_directories(ume_engine PUBLIC src)
target_link_libraries(
  ume_engine
  PUBLIC ume_config ume_plugin_api
  PRIVATE SDL3::SDL3-static glaze spdlog::spdlog wren::wren slang::slang
          ${CMAKE_DL_LIBS})
target_compile_definitions(
  ume_engine PRIVATE UME_PLUGIN_SUFFIX="${CMAKE_SHARED_MODULE_SUFFIX}"
                     UME_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

b_embed(ume_engine src/ume/scripting/ume.wren)

target_sources(ume_engine PRIVATE src/ume/platform/metal/metal_renderer.cpp)
target_include_directories(ume_engine SYSTEM PRIVATE vendor/metal-cpp)

find_library(METAL_LIB Metal REQUIRED)
find_library(FOUNDATION_LIB Foundation REQUIRED)
find_library(QUARTZCORE_LIB QuartzCore REQUIRED)

target_link_libraries(ume_engine PRIVATE ${METAL_LIB} ${FOUNDATION_LIB}
                                         ${QUARTZCORE_LIB})

target_sources(
  ume_engine
  PRIVATE src/ume/plugin/plugin_manifest.cpp src/ume/plugin/plugin_host.cpp
          src/ume/plugin/plugin_params.cpp)

add_executable(ume src/main.cpp)

if(CMAKE_GENERATOR STREQUAL "Xcode")
  set_target_properties(
    ume PROPERTIES XCODE_GENERATE_SCHEME TRUE XCODE_SCHEME_ENVIRONMENT
                                              "MTL_DEBUG_LAYER=1")
endif()

target_link_libraries(ume PRIVATE ume_engine SDL3::Headers)
include(GNUInstallDirs)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  install(TARGETS ume RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
  message(STATUS "ume: install rules omitted for non-Release build "
                 "(embedded resources are loaded from the source tree)")
endif()
