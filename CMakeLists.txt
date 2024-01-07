cmake_minimum_required(VERSION 3.14)

cmake_policy(SET CMP0114 NEW)

project(wiimote-mouse)

include(ExternalProject)
include(FetchContent)

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

# test applications
add_executable(mouse-test src/mouse-test.cpp src/virtualmouse.hpp)
target_link_libraries(mouse-test PkgConfig::evdev PkgConfig::xwiimote)

add_executable(xwiimote-test src/xwiimote-test.cpp)
target_link_libraries(xwiimote-test PkgConfig::evdev PkgConfig::xwiimote)

add_executable(test-maths src/testmaths.cpp src/intlinalg.hpp)

# Actual mouse driver
ExternalProject_Add(
    sockpp
    URL https://github.com/fpagliughi/sockpp/archive/refs/tags/v1.0.0.tar.gz
    URL_HASH SHA256=818477fae6eb29b9daacef71a3d0c494de9e48f6fd267a7fc5a4c332db211bc5
    CMAKE_ARGS 
        "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/sockpp-install"
        -DSOCKPP_BUILD_STATIC=true
        -DSOCKPP_BUILD_SHARED=false
)
ExternalProject_Add_StepTargets(sockpp install)

add_executable(
    xwiimote-mouse-driver 
        src/drivermain.cpp
        src/base.hpp
        src/virtualmouse.hpp
        src/intlinalg.hpp
        src/controlsocket.hpp
        src/controlsocket.cpp
        src/device.hpp
        src/settings.hpp
        src/settings.cpp
        src/stringtools.hpp
        src/stringtools.cpp
        src/driveroptparse.hpp
)
add_dependencies(xwiimote-mouse-driver sockpp-install)
target_include_directories(
    xwiimote-mouse-driver
    PUBLIC
        "${CMAKE_BINARY_DIR}/sockpp-install/include/"
)
target_link_libraries(
    xwiimote-mouse-driver
        PkgConfig::evdev 
        PkgConfig::xwiimote
        "${CMAKE_BINARY_DIR}/sockpp-install/lib/libsockpp.a"
)
add_compile_options(xwiimote-mouse-driver -Wall)
