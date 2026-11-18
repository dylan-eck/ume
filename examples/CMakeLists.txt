cmake_minimum_required(VERSION 3.25)

project(
  ume_examples
  VERSION 0.1.0
  LANGUAGES C CXX)

include("../cmake/UmePlugin.cmake")

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(plugins/shared)
add_subdirectory(plugins/proc_planet)
add_subdirectory(plugins/terrain)

add_subdirectory(../vendor/glm glm SYSTEM)

add_library(ume_examples_config INTERFACE)
target_compile_options(
  ume_examples_config INTERFACE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall
                                -Wextra -Wpedantic -Wimplicit-fallthrough>)

target_compile_features(ume_examples_config INTERFACE cxx_std_23)
target_compile_definitions(
  ume_examples_config INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE
                                GLM_FORCE_EXPLICIT_CTOR GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(ume_examples_config INTERFACE glm::glm)

ume_deploy_plugins(PROJECT planet PLUGINS proc_planet)
ume_deploy_plugins(PROJECT terrain PLUGINS terrain)
