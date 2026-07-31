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
  ume PRIVATE src/main.cpp src/ume/core/logger.cpp src/ume/core/application.cpp
              src/ume/platform/window.cpp src/ume/renderer/renderer.cpp)
target_include_directories(ume PRIVATE src)

target_link_libraries(ume PRIVATE SDL3::SDL3-static glaze sol2::sol2 lua
                                  spdlog::spdlog)

if(APPLE)
  target_sources(ume PRIVATE src/ume/platform/metal/metal_renderer.cpp)
  target_compile_definitions(ume PRIVATE UME_RENDER_BACKEND_METAL)
  target_include_directories(ume PRIVATE vendor/metal-cpp)

  find_library(METAL_LIB Metal REQUIRED)
  find_library(FOUNDATION_LIB Foundation REQUIRED)
  find_library(QUARTZCORE_LIB QuartzCore REQUIRED)

  target_link_libraries(ume PRIVATE ${METAL_LIB} ${FOUNDATION_LIB}
                                    ${QUARTZCORE_LIB})
else()
  target_sources(ume PRIVATE src/ume/platform/vulkan/vulkan_renderer.cpp)
  target_compile_definitions(ume PRIVATE UME_RENDER_BACKEND_VULKAN)

  target_link_libraries(ume PRIVATE Vulkan::Vulkan vk-bootstrap::vk-bootstrap)
endif()

function(add_slang_shader_spirv TARGET SHADER)
  set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}")

  set(SHADER_REL_OUTPUT "generated/${SHADER}.spv")
  set(SHADER_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_REL_OUTPUT}")

  add_custom_command(
    OUTPUT ${SHADER_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/generated"
    COMMAND
      ${SLANGC_EXECUTABLE} ${SHADER_SOURCE} -target spirv -profile spirv_1_4
      -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry
      fragMain -o ${SHADER_OUTPUT}
    DEPENDS ${SHADER_SOURCE}
    COMMENT "Compiling slang shader: ${SHADER}"
    VERBATIM)

  b_embed(${TARGET} ${SHADER_REL_OUTPUT})
endfunction()

function(add_slang_shader_metallib TARGET SHADER)
  set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}")
  set(SHADER_REL_OUTPUT "generated/${SHADER}.metallib")
  set(SHADER_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_REL_OUTPUT}")

  get_filename_component(SHADER_NAME "${SHADER}" NAME)
  set(SHADER_MSL "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.metal")
  set(SHADER_AIR "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.air")

  add_custom_command(
    OUTPUT ${SHADER_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/generated"
    COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCE} -target metal -entry vertMain
            -entry fragMain -o ${SHADER_MSL}
    COMMAND xcrun -sdk macosx metal -gline-tables-only -frecord-sources -c
            ${SHADER_MSL} -o ${SHADER_AIR}
    COMMAND xcrun -sdk macosx metallib ${SHADER_AIR} -o ${SHADER_OUTPUT}
    DEPENDS ${SHADER_SOURCE}
    COMMENT "Compiling slang shader (metallib via xcrun): ${SHADER}"
    VERBATIM)

  b_embed(${TARGET} ${SHADER_REL_OUTPUT})
endfunction()

if(APPLE)
  add_slang_shader_metallib(ume "src/ume/renderer/shaders/triangle.slang")
else()
  add_slang_shader_spirv(ume "src/ume/renderer/shaders/triangle.slang")
endif()
