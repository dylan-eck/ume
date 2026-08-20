cmake_minimum_required(VERSION 3.25)

project(
  ume
  VERSION 0.1.0
  LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/dependencies.cmake)

if(APPLE)
  set(UME_DEFAULT_RENDER_BACKEND "metal")
else()
  set(UME_DEFAULT_RENDER_BACKEND "vulkan")
endif()

set(UME_RENDER_BACKEND
    "${UME_DEFAULT_RENDER_BACKEND}"
    CACHE STRING "Renderer backend to build (metal or vulkan)")
set_property(CACHE UME_RENDER_BACKEND PROPERTY STRINGS metal vulkan)

if(NOT UME_RENDER_BACKEND STREQUAL "metal" AND NOT UME_RENDER_BACKEND STREQUAL
                                               "vulkan")
  message(
    FATAL_ERROR
      "UME_RENDER_BACKEND must be 'metal' or 'vulkan', got ${UME_RENDER_BACKEND}"
  )
endif()

if(UME_RENDER_BACKEND STREQUAL "metal" AND NOT APPLE)
  message(
    FATAL_ERROR "UME_RENDER_BACKEND=metal is only supported on Apple platforms")
endif()

message(STATUS "ume: building with ${UME_RENDER_BACKEND} render backend")

add_library(ume_config INTERFACE)
target_compile_features(ume_config INTERFACE cxx_std_23)
target_compile_definitions(
  ume_config
  INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_EXPLICIT_CTOR
            GLM_ENABLE_EXPERIMENTAL $<$<CONFIG:Debug>:UME_ENABLE_LOGGING>)
target_link_libraries(ume_config INTERFACE glm::glm)

add_library(ume_plugin_config INTERFACE)
target_compile_options(
  ume_plugin_config
  INTERFACE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
            $<$<CXX_COMPILER_ID:MSVC>:/W4>)

add_library(ume_plugin_api INTERFACE)
target_include_directories(ume_plugin_api INTERFACE include)

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
  src/ume/core/logger.cpp src/ume/core/application.cpp
  src/ume/core/dynamic_library.cpp src/ume/platform/window.cpp
  src/ume/renderer/renderer.cpp src/ume/scripting/script_engine.cpp)
target_compile_options(
  ume_engine PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra
                     -Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4>)
target_include_directories(ume_engine PUBLIC src)
target_link_libraries(
  ume_engine
  PUBLIC ume_config ume_plugin_api
  PRIVATE SDL3::SDL3-static glaze spdlog::spdlog tinygltf::tinygltf wren::wren
          ${CMAKE_DL_LIBS})
target_compile_definitions(
  ume_engine PRIVATE UME_PLUGIN_SUFFIX="${CMAKE_SHARED_MODULE_SUFFIX}")

b_embed(ume_engine src/ume/scripting/ume.wren)

if(UME_RENDER_BACKEND STREQUAL "metal")
  target_sources(ume_engine PRIVATE src/ume/platform/metal/metal_renderer.cpp)
  target_compile_definitions(ume_engine PRIVATE UME_RENDER_BACKEND_METAL)
  target_include_directories(ume_engine PRIVATE vendor/metal-cpp)

  find_library(METAL_LIB Metal REQUIRED)
  find_library(FOUNDATION_LIB Foundation REQUIRED)
  find_library(QUARTZCORE_LIB QuartzCore REQUIRED)

  target_link_libraries(ume_engine PRIVATE ${METAL_LIB} ${FOUNDATION_LIB}
                                           ${QUARTZCORE_LIB})
else()
  target_sources(ume_engine PRIVATE src/ume/platform/vulkan/vulkan_renderer.cpp)
  target_compile_definitions(ume_engine PRIVATE UME_RENDER_BACKEND_VULKAN)
  target_link_libraries(ume_engine PRIVATE Vulkan::Vulkan
                                           vk-bootstrap::vk-bootstrap)
endif()

function(add_slang_shader TARGET SHADER)
  set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}")

  if(UME_RENDER_BACKEND STREQUAL "metal")
    set(SHADER_REL_OUTPUT "generated/${SHADER}.metallib")
    set(SHADER_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_REL_OUTPUT}")

    get_filename_component(SHADER_NAME "${SHADER}" NAME)
    set(SHADER_MSL "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.metal")
    set(SHADER_AIR "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.air")

    add_custom_command(
      OUTPUT ${SHADER_OUTPUT}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              "${CMAKE_CURRENT_SOURCE_DIR}/generated"
      COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCE} -target metal -entry
              vertMain -entry fragMain -o ${SHADER_MSL}
      COMMAND xcrun -sdk macosx metal -gline-tables-only -frecord-sources -c
              ${SHADER_MSL} -o ${SHADER_AIR}
      COMMAND xcrun -sdk macosx metallib ${SHADER_AIR} -o ${SHADER_OUTPUT}
      DEPENDS ${SHADER_SOURCE}
      COMMENT "Compiling Metal shader: ${SHADER}"
      VERBATIM)
  else()
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
      COMMENT "Compiling SPIR-V shader: ${SHADER}"
      VERBATIM)
  endif()

  b_embed(${TARGET} ${SHADER_REL_OUTPUT})
endfunction()

add_slang_shader(ume_engine "src/ume/renderer/shaders/triangle.slang")

set(UME_PLUGIN_LINK_TYPE
    "STATIC"
    CACHE STRING "STATIC or MODULE")
set_property(CACHE UME_PLUGIN_LINK_TYPE PROPERTY STRINGS STATIC MODULE)

target_sources(
  ume_engine
  PRIVATE src/ume/plugin/plugin_host.cpp src/ume/plugin/plugin_params.cpp
          src/ume/plugin/builtin_plugins.cpp)

add_subdirectory(plugins/proc_planet)

add_executable(ume src/main.cpp)

if(UME_PLUGIN_LINK_TYPE STREQUAL "STATIC")
  target_compile_definitions(ume_engine PRIVATE UME_PLUGIN_STATIC)
else()
  add_dependencies(ume proc_planet)
endif()

target_link_libraries(ume PRIVATE ume_engine)
if(UME_PLUGIN_LINK_TYPE STREQUAL "STATIC")
  target_link_libraries(ume PRIVATE proc_planet)
endif()

include(GNUInstallDirs)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  install(TARGETS ume RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
  message(STATUS "ume: install rules omitted for non-Release build "
                 "(embedded resources are loaded from the source tree)")
endif()
