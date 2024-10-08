if(MSVC OR (WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
  # avoid "cannot open file 'LIBC.lib'" error
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:LIBC.LIB")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBC.LIB")
endif()


if (SHOULD_BUILD_PLUGIN_COLORENGINES)
    add_subdirectory( colorengines )
endif ()

if (SHOULD_BUILD_PLUGIN_MUSICSHAPE)
    add_subdirectory( musicshape )
endif ()

if (TARGET Qt6::WebEngineWidgets)
  add_subdirectory(webshape)
endif ()

if (SHOULD_BUILD_PLUGIN_ARTISTICTEXTSHAPE)
    add_subdirectory( artistictextshape )
endif ()

if (SHOULD_BUILD_PLUGIN_DOCKERS)
    add_subdirectory( dockers )
endif ()

if (SHOULD_BUILD_PLUGIN_TEXTEDITING)
    add_subdirectory( textediting )
endif()

if (SHOULD_BUILD_PLUGIN_DEFAULTTOOLS)
    add_subdirectory( defaultTools )
endif ()

if (SHOULD_BUILD_PLUGIN_PATHSHAPES)
    add_subdirectory( pathshapes )
endif ()

if (SHOULD_BUILD_PLUGIN_VARIABLES)
    add_subdirectory( variables )
endif ()

if (SHOULD_BUILD_PLUGIN_CHARTSHAPE)
    add_subdirectory( chartshape )
endif ()

if (SHOULD_BUILD_PLUGIN_PICTURESHAPE)
    add_subdirectory( pictureshape )
endif ()

if (SHOULD_BUILD_PLUGIN_TEXTSHAPE)
    add_subdirectory( textshape )
endif()

if (SHOULD_BUILD_PLUGIN_PLUGINSHAPE)
    add_subdirectory( pluginshape )
endif ()

if (SHOULD_BUILD_PLUGIN_FORMULASHAPE)
    add_subdirectory( formulashape )
endif ()

if (SHOULD_BUILD_PLUGIN_VECTORSHAPE)
    # ??? old comment: as the vector shape depends on filters subdir (not really intuitive) lets allow disabling
    add_subdirectory( vectorshape )
endif ()

if (SHOULD_BUILD_PLUGIN_VIDEOSHAPE)
    add_subdirectory( videoshape )
endif ()

if(SHOULD_BUILD_PLUGIN_SEMANTICITEMS)
    message(WARNING "--- semanicitems is not built. It needs to be ported from kdelibs4")
    #add_subdirectory( semanticitems )
endif()

if(SHOULD_BUILD_PLUGIN_SHAPEFILTEREFFECTS)
    add_subdirectory( shapefiltereffects )
endif()

if(SHOULD_BUILD_PLUGIN_STENCILSDOCKER)
    add_subdirectory( stencilsdocker)
endif()

if(SHOULD_BUILD_PLUGIN_KARBONPLUGINS)
    add_subdirectory( karbonplugins )
endif()

#add_subdirectory( videoshape )
#add_subdirectory( commentshape )

add_subdirectory( staging )
