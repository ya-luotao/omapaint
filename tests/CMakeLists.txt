find_package(Qt6 REQUIRED COMPONENTS Test Gui Qml)

set(OMAPAINT_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/document.cpp
    ${CMAKE_SOURCE_DIR}/src/image_io.cpp
    ${CMAKE_SOURCE_DIR}/src/canvas_view.cpp
    ${CMAKE_SOURCE_DIR}/src/selection_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/clipboard.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/tool.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/fill_tool.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/pixelate_tool.cpp
    ${CMAKE_SOURCE_DIR}/src/text_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/commands/draw_command.cpp
    ${CMAKE_SOURCE_DIR}/src/commands/image_command.cpp
)

function(omapaint_add_test name source)
    qt_add_executable(${name} ${source} ${OMAPAINT_CORE_SOURCES})
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${name} PRIVATE Qt6::Test Qt6::Gui Qt6::Qml)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

omapaint_add_test(tst_document document/tst_document.cpp)
omapaint_add_test(tst_image_io image_io/tst_image_io.cpp)
omapaint_add_test(tst_tools tools/tst_tools.cpp)
omapaint_add_test(tst_undo undo/tst_undo.cpp)
omapaint_add_test(tst_canvas_view canvas_view/tst_canvas_view.cpp)
omapaint_add_test(tst_selection selection/tst_selection.cpp)
omapaint_add_test(tst_commands commands/tst_commands.cpp)
omapaint_add_test(tst_annotate annotate/tst_annotate.cpp)
omapaint_add_test(tst_theme theme/tst_theme.cpp)
target_sources(tst_theme PRIVATE ${CMAKE_SOURCE_DIR}/src/theme.cpp)

# The CLI contract agents rely on: --help must document every entry point.
add_test(NAME tst_help COMMAND omapaint --help)
set_tests_properties(tst_help PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    PASS_REGULAR_EXPRESSION "--annotate <file>")
omapaint_add_test(tst_perf perf/tst_perf.cpp)
omapaint_add_test(tst_ruby ruby/tst_ruby.cpp)
# The portrait test needs Ruby's block art, which normally rides in the
# application binary's resources.
qt_add_resources(tst_ruby "ruby_art"
    PREFIX "/"
    BASE ${CMAKE_SOURCE_DIR}/assets
    FILES ${CMAKE_SOURCE_DIR}/assets/ruby.txt
)
