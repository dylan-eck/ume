include_guard(GLOBAL)
set(_UME_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

add_library(ume_plugin_api INTERFACE)
add_library(ume::plugin_api ALIAS ume_plugin_api)
target_include_directories(ume_plugin_api INTERFACE "${_UME_ROOT}/include")

function(ume_add_plugin name)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;ASSETS")
  add_library(${name} MODULE ${arg_SOURCES})
  target_link_libraries(${name} PRIVATE ume::plugin_api)

  set(assets plugin.toml ${arg_ASSETS})

  set_target_properties(
    ${name}
    PROPERTIES PREFIX ""
               C_VISIBILITY_PRESET hidden
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN ON
               UME_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
               UME_PLUGIN_ASSETS "${assets}")
endfunction()

function(ume_deploy_plugins)
  cmake_parse_arguments(PARSE_ARGV 0 arg "" "PROJECT" "PLUGINS")
  if(arg_UNPARSED_ARGUMENTS
     OR NOT arg_PROJECT
     OR NOT arg_PLUGINS)
    message(
      FATAL_ERROR "usage: ume_deploy_plugins(PROJECT <dir> PLUGINS <target>...)"
    )
  endif()

  cmake_path(
    ABSOLUTE_PATH
    arg_PROJECT
    BASE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}"
    NORMALIZE
    OUTPUT_VARIABLE
    project_dir)
  file(RELATIVE_PATH project_rel "${CMAKE_SOURCE_DIR}" "${project_dir}")
  string(MAKE_C_IDENTIFIER "${project_rel}" project_id)

  foreach(plugin IN LISTS arg_PLUGINS)
    if(NOT TARGET ${plugin})
      message(FATAL_ERROR "ume_deploy_plugins: no target named '${plugin}'")
    endif()
    get_target_property(plugin_dir ${plugin} UME_PLUGIN_DIR)
    if(NOT plugin_dir)
      message(
        FATAL_ERROR
          "ume_deploy_plugins: '${plugin}' was not created with ume_add_plugin")
    endif()
    get_target_property(assets ${plugin} UME_PLUGIN_ASSETS)

    set(dest "${project_dir}/plugins/${plugin}")
    set(outputs "")

    set(binary "${dest}/${plugin}${CMAKE_SHARED_MODULE_SUFFIX}")
    add_custom_command(
      OUTPUT "${binary}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${dest}"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${plugin}>"
              "${binary}.tmp"
      COMMAND ${CMAKE_COMMAND} -E rename "${binary}.tmp" "${binary}"
      DEPENDS ${plugin}
      COMMENT "Deploying ${plugin} to ${project_rel}"
      VERBATIM)
    list(APPEND outputs "${binary}")

    foreach(asset IN LISTS assets)
      set(src "${plugin_dir}/${asset}")
      set(dst "${dest}/${asset}")
      cmake_path(GET dst PARENT_PATH dst_dir)
      cmake_path(GET asset EXTENSION LAST_ONLY ext)

      if(ext STREQUAL ".slang")
        # symlink so shader edits hot-reload without a rebuild. rm first:
        # create_symlink won't replace an existing regular file
        add_custom_command(
          OUTPUT "${dst}"
          COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
          COMMAND ${CMAKE_COMMAND} -E rm -f "${dst}"
          COMMAND ${CMAKE_COMMAND} -E create_symlink "${src}" "${dst}"
          DEPENDS "${src}"
          COMMENT "Linking ${plugin}/${asset} into ${project_rel}"
          VERBATIM)
      else()
        add_custom_command(
          OUTPUT "${dst}"
          COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
          COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src}" "${dst}"
          DEPENDS "${src}"
          COMMENT "Copying ${plugin}/${asset} into ${project_rel}"
          VERBATIM)
      endif()
      list(APPEND outputs "${dst}")
    endforeach()

    add_custom_target(ume_deploy_${project_id}_${plugin} ALL DEPENDS ${outputs})
  endforeach()
endfunction()
