project(gemini)

set(gemini_SRCS
    GeminiMainWindow.cpp
)

add_library(gemini SHARED ${gemini_SRCS})
generate_export_header(gemini)
target_link_libraries(gemini
    Qt5::Quick
    Qt5::Gui
    Qt5::Core
    Qt5::Widgets
)

set_target_properties(gemini PROPERTIES
    VERSION ${GENERIC_CALLIGRA_LIB_VERSION} SOVERSION ${GENERIC_CALLIGRA_LIB_SOVERSION}
)

install(TARGETS gemini ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
