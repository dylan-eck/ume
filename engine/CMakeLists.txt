add_library(ume STATIC)

target_sources(ume PRIVATE src/application.cpp)

target_include_directories(ume PUBLIC include)
