cmake_minimum_required(VERSION 3.21)
project(omapaint VERSION 0.2.0 LANGUAGES CXX)

find_package(Qt6 6.9 REQUIRED COMPONENTS Gui Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(omapaint
    src/main.cpp
    src/document.cpp src/document.h
    src/image_io.cpp src/image_io.h
    src/canvas_item.cpp src/canvas_item.h
    src/canvas_view.cpp src/canvas_view.h
    src/selection_controller.cpp src/selection_controller.h
    src/clipboard.cpp src/clipboard.h
    src/tools/tool.cpp src/tools/tool.h
    src/tools/pencil_tool.h
    src/tools/brush_tool.h
    src/tools/eraser_tool.h
    src/tools/line_tool.h
    src/tools/arrow_tool.h
    src/tools/rectangle_tool.h
    src/tools/ellipse_tool.h
    src/tools/fill_tool.cpp src/tools/fill_tool.h
    src/tools/pixelate_tool.cpp src/tools/pixelate_tool.h
    src/text_renderer.cpp src/text_renderer.h
    src/commands/draw_command.cpp src/commands/draw_command.h
    src/commands/image_command.cpp src/commands/image_command.h
    src/theme.cpp src/theme.h
    src/demo_driver.cpp src/demo_driver.h
)

target_compile_definitions(omapaint PRIVATE OMAPAINT_VERSION="${PROJECT_VERSION}")

# Tool icons and the app logo, drawn by OmaPaint itself (devtools/draw_icons).
qt_add_resources(omapaint "assets"
    PREFIX "/"
    BASE assets
    FILES
        assets/omapaint.png
        assets/icons/select.png
        assets/icons/pencil.png
        assets/icons/brush.png
        assets/icons/eraser.png
        assets/icons/line.png
        assets/icons/arrow.png
        assets/icons/rectangle.png
        assets/icons/ellipse.png
        assets/icons/fill.png
        assets/icons/text.png
        assets/icons/pixelate.png
        assets/icons/eyedropper.png
)

qt_add_qml_module(omapaint
    URI OmaPaint
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/EditorToolBar.qml
        qml/ToolPanel.qml
        qml/ColorPalette.qml
        qml/StatusBar.qml
)

target_include_directories(omapaint PRIVATE src)
target_link_libraries(omapaint PRIVATE Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

include(GNUInstallDirs)
install(TARGETS omapaint RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS packaging/omapaint-edit DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/omapaint.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/omapaint.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)

enable_testing()
add_subdirectory(tests)

# Dev-only helper that paints the README preview image with OmaPaint's tools.
qt_add_executable(draw_logo
    devtools/draw_logo.cpp
    src/tools/tool.cpp
    src/tools/fill_tool.cpp
)
target_include_directories(draw_logo PRIVATE src)
target_link_libraries(draw_logo PRIVATE Qt6::Gui)

# Dev-only helper that paints the committed tool icons and app logo with
# OmaPaint's tools (dogfood; see assets/icons/).
qt_add_executable(draw_icons
    devtools/draw_icons.cpp
    src/tools/tool.cpp
    src/tools/fill_tool.cpp
    src/tools/pixelate_tool.cpp
    src/text_renderer.cpp
)
target_include_directories(draw_icons PRIVATE src)
target_link_libraries(draw_icons PRIVATE Qt6::Gui)
