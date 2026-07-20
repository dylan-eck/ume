add_library(ume_engine STATIC)
add_library(ume::engine ALIAS ume_engine)
target_sources(ume_engine PRIVATE src/application.cpp src/window.cpp
                                  src/renderer.cpp)
target_include_directories(ume_engine PUBLIC include)

find_package(Vulkan REQUIRED)
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
target_link_libraries(ume_engine PRIVATE SDL3::SDL3-static Vulkan::Vulkan)

add_library(ume_main STATIC src/main.cpp)
add_library(ume::main ALIAS ume_main)
target_link_libraries(ume_main PUBLIC ume::engine)
