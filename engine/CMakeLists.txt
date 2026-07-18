add_library(ume STATIC)

target_sources(ume PRIVATE src/application.cpp src/platform/window.cpp)

target_include_directories(ume PUBLIC include)

target_link_libraries(ume PRIVATE SDL3::SDL3-static)
