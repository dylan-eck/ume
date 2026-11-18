ume_add_plugin(terrain SOURCES src/plugin.cpp src/terrain.cpp)

target_link_libraries(terrain PRIVATE ume_examples_config FastNoise)
