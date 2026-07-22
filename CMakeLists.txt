cmake_minimum_required(VERSION 3.25)

project(
  ume
  VERSION 0.1.0
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/dependencies.cmake)

add_executable(ume)
target_compile_definitions(ume PRIVATE $<$<CONFIG:Debug>:UME_ENABLE_LOGGING>)

target_sources(
  ume
  PRIVATE src/main.cpp src/ume/core/logger.cpp src/ume/core/application.cpp
          src/ume/platform/window.cpp src/ume/renderer/renderer.cpp
          src/ume/platform/vulkan/vulkan_renderer.cpp)
target_include_directories(ume PRIVATE src)
target_link_libraries(ume PRIVATE SDL3::SDL3-static glaze sol2::sol2 lua
                                  Vulkan::Vulkan spdlog::spdlog)
