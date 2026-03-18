cmake_minimum_required(VERSION 3.16)
project(SafeSocketGUI)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)
find_package(SDL3 REQUIRED)

add_executable(SafeSocketGUI
    gui/src/main_gui.cpp
    gui/src/security.cpp
)

target_include_directories(SafeSocketGUI PRIVATE gui/src)

target_link_libraries(SafeSocketGUI
    SDL3::SDL3
    OpenSSL::SSL
    OpenSSL::Crypto
)
