find_package(Qt6 REQUIRED COMPONENTS Test Gui Qml)

set(OMAPAINT_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/document.cpp
    ${CMAKE_SOURCE_DIR}/src/image_io.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/tool.cpp
    ${CMAKE_SOURCE_DIR}/src/commands/draw_command.cpp
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
