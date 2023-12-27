cmake_minimum_required(VERSION 3.12)
project(wiimote-mouse)

find_package(PkgConfig)

pkg_search_module(
    evdev REQUIRED 
    IMPORTED_TARGET
    libevdev>=1.12
)
pkg_search_module(
    xwiimote REQUIRED 
    IMPORTED_TARGET
    libxwiimote
)

add_executable(mouse-test src/mouse-test.cpp)
target_link_libraries(mouse-test PkgConfig::evdev PkgConfig::xwiimote)
