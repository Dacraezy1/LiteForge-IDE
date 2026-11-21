add_executable(liteforge_tests test_core.cpp)
target_link_libraries(liteforge_tests PRIVATE liteforge_objs)
add_test(NAME CoreTests COMMAND liteforge_tests)

# GUI tests run headless (no display needed).
add_executable(liteforge_gui_tests test_gui.cpp)
target_link_libraries(liteforge_gui_tests PRIVATE liteforge_objs)
add_test(NAME GuiTests COMMAND liteforge_gui_tests)
set_tests_properties(GuiTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
