add_definitions(-DTRANSLATION_DOMAIN=\"calligra_semanticitem_location\")

if(Marble_FOUND)
    add_definitions( -DCAN_USE_MARBLE )
endif()

include_directories(
    ${KORDF_INCLUDES}
)
set(calligra_semanticitem_location_LIB_SRCS
    KoLocationSemanticItemFactory.cpp
    KoLocationSemanticItemPlugin.cpp
    KoRdfLocation.cpp
    KoRdfLocationEditWidget.cpp
    KoRdfLocationTreeWidgetItem.cpp
)

if(Marble_FOUND)
    ki18n_wrap_ui( calligra_semanticitem_location_LIB_SRCS
        KoRdfLocationEditWidgetMarble.ui
        KoRdfLocationViewWidgetMarble.ui
    )
else()
    ki18n_wrap_ui( calligra_semanticitem_location_LIB_SRCS
        KoRdfLocationEditWidget.ui
        KoRdfLocationViewWidget.ui
    )
endif()

add_library(calligra_semanticitem_location MODULE ${calligra_semanticitem_location_LIB_SRCS})

if( Marble_FOUND )
    target_link_libraries(calligra_semanticitem_location Marble)
endif()

target_link_libraries(calligra_semanticitem_location   ${KORDF_LIBS} KF5::I18n KF5::KDELibs4Support)
calligra_semanticitem_desktop_to_json(calligra_semanticitem_location calligra_semanticitem_location.desktop)

install(TARGETS calligra_semanticitem_location  DESTINATION ${KDE_INSTALL_PLUGINDIR}/calligra/semanticitems)
