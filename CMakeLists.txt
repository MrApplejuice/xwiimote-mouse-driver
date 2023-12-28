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

# Experimental test applications
add_executable(mouse-test src/mouse-test.cpp)
target_link_libraries(mouse-test PkgConfig::evdev PkgConfig::xwiimote)

add_executable(xwiimote-test src/xwiimote-test.cpp)
target_link_libraries(xwiimote-test PkgConfig::evdev PkgConfig::xwiimote)

# Actual mouse driver
add_executable(
    xwiimote-mouse-driver 
        src/drivermain.cpp
        src/base.hpp
)
target_link_libraries(xwiimote-mouse-driver  PkgConfig::evdev PkgConfig::xwiimote)
add_compile_options(xwiimote-mouse-driver  -Wall)
