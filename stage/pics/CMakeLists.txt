add_subdirectory( animations )

install( FILES
         layout-elements.svg
         DESTINATION ${DATA_INSTALL_DIR}/calligrastage/pics)

install( FILES
    pen.png
    DESTINATION ${DATA_INSTALL_DIR}/calligrastage/cursors
)

ecm_install_icons( ICONS
    16-actions-pause-to-black-screen.png
    16-actions-highlight-pointer-spot.png

    DESTINATION ${DATA_INSTALL_DIR}/calligrastage/icons
    THEME hicolor
)

ecm_install_icons( ICONS
    16-apps-calligrastage.png
    22-apps-calligrastage.png
    32-apps-calligrastage.png
    48-apps-calligrastage.png
    64-apps-calligrastage.png
    128-apps-calligrastage.png
    256-apps-calligrastage.png
    512-apps-calligrastage.png
    1024-apps-calligrastage.png
    sc-apps-calligrastage.svgz

    DESTINATION ${ICON_INSTALL_DIR}
    THEME hicolor
)
