cmake_minimum_required(VERSION 3.16)
project(countdown-widget VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(LayerShellQt QUIET)   # KDE's layer-shell-qt: pins the widget to the desktop on Wayland

add_executable(countdown-widget main.cpp)
target_link_libraries(countdown-widget PRIVATE Qt6::Widgets)

if(LayerShellQt_FOUND)
    message(STATUS "LayerShellQt ${LayerShellQt_VERSION} found: desktop-layer placement enabled")
    target_link_libraries(countdown-widget PRIVATE LayerShellQt::Interface)
    target_compile_definitions(countdown-widget PRIVATE
        HAVE_LAYERSHELLQT
        LAYERSHELLQT_VERSION_MAJOR=${LayerShellQt_VERSION_MAJOR})
else()
    message(WARNING "LayerShellQt not found: the widget will be a normal frameless window on Wayland")
endif()

include(GNUInstallDirs)
install(TARGETS countdown-widget DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES countdown-widget.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
