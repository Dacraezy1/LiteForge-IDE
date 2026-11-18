add_executable(liteforge_tests
    test_core.cpp
    ${CMAKE_SOURCE_DIR}/include/liteforge/core/Document.hpp
    ${CMAKE_SOURCE_DIR}/include/liteforge/core/Theme.hpp
    ${CMAKE_SOURCE_DIR}/src/core/FileUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Document.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Theme.cpp
)

target_include_directories(liteforge_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(liteforge_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

add_test(NAME CoreTests COMMAND liteforge_tests)
