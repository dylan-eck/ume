add_library(ume_engine STATIC)
add_library(ume::engine ALIAS ume_engine)
target_sources(ume_engine PRIVATE src/application.cpp src/window.cpp
                                  src/renderer.cpp)
target_include_directories(ume_engine PUBLIC include)

find_package(Vulkan REQUIRED)
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
target_link_libraries(ume_engine PRIVATE SDL3::SDL3-static Vulkan::Vulkan)

add_library(ume_entry STATIC src/entry.cpp)
add_library(ume::entry ALIAS ume_entry)
target_link_libraries(ume_entry PUBLIC ume::engine)
