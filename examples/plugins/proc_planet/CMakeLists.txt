ume_add_plugin(
  proc_planet
  SOURCES src/plugin.cpp src/proc_planet.cpp
  ASSETS src/shaders/ocean.slang src/shaders/sun.slang
         src/shaders/atmosphere.slang)

target_link_libraries(proc_planet PRIVATE ume_examples_config FastNoise)
