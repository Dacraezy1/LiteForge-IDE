cmake_minimum_required(VERSION 3.20)
project(LiteForge
    VERSION 1.2.0
    DESCRIPTION "Featherweight, zero-bloat native C++ IDE"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(ENABLE_TESTING "Build and register the test suite" ON)
option(LITEFORGE_LTO  "Use link-time optimisation when the toolchain supports it" ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

# --- Size / speed flags -------------------------------------------------------
# -O2 (not -O3): smaller code, fewer i-cache misses, same real-world speed for a
# GUI app. Dead code and data are dropped at link time.
set(LITEFORGE_CXX_FLAGS "")
set(LITEFORGE_LINK_FLAGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND LITEFORGE_CXX_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND LITEFORGE_CXX_FLAGS -O2 -ffunction-sections -fdata-sections
                    -fvisibility=hidden -fvisibility-inlines-hidden)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            list(APPEND LITEFORGE_LINK_FLAGS -Wl,--gc-sections -Wl,--as-needed -Wl,-O1 -Wl,-z,relro -Wl,-z,now)
        elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD|OpenBSD|NetBSD|DragonFly")
            list(APPEND LITEFORGE_LINK_FLAGS -Wl,--gc-sections)
        endif()
        if(LITEFORGE_LTO)
            include(CheckIPOSupported)
            check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
            if(ipo_supported)
                set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
            endif()
        endif()
    endif()
endif()

# --- Sources ------------------------------------------------------------------
set(LITEFORGE_SOURCES
    # core
    src/core/Config.cpp
    src/core/Theme.cpp
    src/core/Document.cpp
    src/core/FileUtils.cpp
    src/core/Project.cpp
    src/core/ProjectTemplates.cpp
    src/core/ProfileManager.cpp
    src/core/Keybindings.cpp
    # git
    src/git/GitService.cpp
    # editor
    src/editor/SyntaxHighlighter.cpp
    src/editor/LineNumberGutter.cpp
    src/editor/FindReplaceWidget.cpp
    src/editor/CodeEditor.cpp
    src/editor/TabManager.cpp
    src/editor/EditorSplitter.cpp
    # ui
    src/ui/FileExplorer.cpp
    src/ui/CommandPalette.cpp
    src/ui/TerminalPanel.cpp
    src/ui/DiagnosticsPanel.cpp
    src/ui/FindInFilesDialog.cpp
    src/ui/StatusBar.cpp
    src/ui/SettingsDialog.cpp
    src/ui/AboutDialog.cpp
    src/ui/GitPanel.cpp
    src/ui/DiffViewerDialog.cpp
    src/ui/WelcomeScreen.cpp
    src/ui/CompletionPopup.cpp
    src/ui/DebuggerPanel.cpp
    src/ui/MainWindow.cpp
    # build / lsp
    src/build/BuildSystem.cpp
    src/build/Debugger.cpp
    src/lsp/LSPClient.cpp
)

# Headers listed so AUTOMOC sees them.
file(GLOB_RECURSE LITEFORGE_HEADERS CONFIGURE_DEPENDS "include/*.hpp")

# Everything except main() lives in an object library; the app and the tests
# both link it, so tests exercise the real code.
add_library(liteforge_objs OBJECT ${LITEFORGE_SOURCES} ${LITEFORGE_HEADERS})
target_include_directories(liteforge_objs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(liteforge_objs PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)
target_compile_options(liteforge_objs PRIVATE ${LITEFORGE_CXX_FLAGS})
target_compile_definitions(liteforge_objs PUBLIC
    LITEFORGE_VERSION="${PROJECT_VERSION}"
    QT_NO_CAST_TO_ASCII
    QT_NO_FOREACH
)
set_target_properties(liteforge_objs PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(liteforge src/main.cpp)
target_link_libraries(liteforge PRIVATE liteforge_objs)
target_compile_options(liteforge PRIVATE ${LITEFORGE_CXX_FLAGS})
target_link_options(liteforge PRIVATE ${LITEFORGE_LINK_FLAGS})

# --- Tests --------------------------------------------------------------------
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Install ------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS liteforge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES assets/liteforge.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/liteforge.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES assets/icon.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME liteforge.svg)
install(DIRECTORY assets/themes/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/liteforge/example-themes
    FILES_MATCHING PATTERN "*.json")

# --- CPack (.tar.gz / .tar.xz / .deb / .rpm) -----------------------------------
set(CPACK_PACKAGE_NAME "liteforge")
set(CPACK_PACKAGE_VENDOR "LiteForge Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Featherweight, zero-bloat native C++ IDE")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Dacraezy1 <https://github.com/Dacraezy1/LiteForge-IDE/issues>")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Dacraezy1/LiteForge-IDE")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dacraezy1 <https://github.com/Dacraezy1/LiteForge-IDE/issues>")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-only")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_AUTOREQPROV "yes")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

set(CPACK_GENERATOR "TGZ;TXZ")
include(CPack)
