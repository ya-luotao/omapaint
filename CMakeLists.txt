cmake_minimum_required(VERSION 3.21)
project(omapaint VERSION 0.0.1 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(omapaint
    src/main.cpp
    src/document.cpp src/document.h
    src/image_io.cpp src/image_io.h
    src/canvas_item.cpp src/canvas_item.h
    src/canvas_view.cpp src/canvas_view.h
    src/tools/tool.cpp src/tools/tool.h
    src/tools/pencil_tool.h
    src/tools/brush_tool.h
    src/tools/eraser_tool.h
    src/tools/line_tool.h
    src/tools/rectangle_tool.h
    src/tools/ellipse_tool.h
    src/tools/fill_tool.cpp src/tools/fill_tool.h
    src/commands/draw_command.cpp src/commands/draw_command.h
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

enable_testing()
add_subdirectory(tests)
