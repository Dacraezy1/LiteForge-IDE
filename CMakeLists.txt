cmake_minimum_required(VERSION 3.20)
project(LiteForge
    VERSION 1.0.0
    DESCRIPTION "Featherweight, zero-bloat native C++ IDE"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

# Compiler optimization flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-O3)
        # Link Time Optimization
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
        if(ipo_supported)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        endif()
    endif()
endif()

# Headers (needed for Qt AUTOMOC)
file(GLOB_RECURSE ALL_HEADERS "include/*.hpp")

# Source files
set(CORE_SOURCES
    src/core/Config.cpp
    src/core/Theme.cpp
    src/core/Document.cpp
    src/core/FileUtils.cpp
    src/core/Project.cpp
)

set(EDITOR_SOURCES
    src/editor/SyntaxHighlighter.cpp
    src/editor/LineNumberGutter.cpp
    src/editor/FindReplaceWidget.cpp
    src/editor/CodeEditor.cpp
    src/editor/TabManager.cpp
)

set(UI_SOURCES
    src/ui/FileExplorer.cpp
    src/ui/CommandPalette.cpp
    src/ui/TerminalPanel.cpp
    src/ui/DiagnosticsPanel.cpp
    src/ui/FindInFilesDialog.cpp
    src/ui/StatusBar.cpp
    src/ui/SettingsDialog.cpp
    src/ui/AboutDialog.cpp
    src/ui/MainWindow.cpp
)

set(BUILD_SOURCES
    src/build/BuildSystem.cpp
)

set(LSP_SOURCES
    src/lsp/LSPClient.cpp
)

add_executable(liteforge
    src/main.cpp
    ${ALL_HEADERS}
    ${CORE_SOURCES}
    ${EDITOR_SOURCES}
    ${UI_SOURCES}
    ${BUILD_SOURCES}
    ${LSP_SOURCES}
)

target_include_directories(liteforge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(liteforge PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Testing
enable_testing()
add_subdirectory(tests)

# Installation rules
include(GNUInstallDirs)
install(TARGETS liteforge
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES assets/liteforge.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES assets/icon.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME liteforge.svg
)

# -------------------------------------------------------------
# CPack Packaging Configuration (.deb, .rpm, .tar.gz, .tar.xz)
# -------------------------------------------------------------
set(CPACK_PACKAGE_NAME "liteforge")
set(CPACK_PACKAGE_VENDOR "LiteForge Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Featherweight, zero-bloat native C++ IDE")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "LiteForge Team <contact@liteforge.org>")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Dacraezy1/LiteForge-IDE")

# Debian (.deb)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "LiteForge Team <contact@liteforge.org>")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# RPM (.rpm)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_AUTOREQPROV "yes")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

# Default Generators
set(CPACK_GENERATOR "TGZ;TXZ")

include(CPack)
