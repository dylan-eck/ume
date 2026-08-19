add_library(proc_planet ${UME_PLUGIN_LINK_TYPE} src/plugin.cpp
                        src/proc_planet.cpp)
target_link_libraries(proc_planet PRIVATE ume_config ume_plugin_config
                                          ume_plugin_api)
set_target_properties(
  proc_planet
  PROPERTIES CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN ON
             POSITION_INDEPENDENT_CODE ON)

if(APPLE)
  target_link_options(proc_planet PRIVATE -Wl,-undefined,error)
elseif(NOT MSVC)
  target_link_options(proc_planet PRIVATE -Wl,--no-undefined)
endif()

if(UME_PLUGIN_LINK_TYPE STREQUAL "STATIC")
  target_compile_definitions(proc_planet PRIVATE UME_PLUGIN_STATIC)
else()
  set_target_properties(
    proc_planet
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${CMAKE_SOURCE_DIR}/sandbox/plugins/proc_planet" PREFIX "")
endif()
