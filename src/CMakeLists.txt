cmake_minimum_required(VERSION 3.16)
project(SnatchforceGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(SnatchforceGUI WIN32
    main.cpp
    snatchforce_core.cpp
)

target_link_libraries(SnatchforceGUI PRIVATE Qt6::Widgets)
