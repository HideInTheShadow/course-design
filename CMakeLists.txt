cmake_minimum_required(VERSION 3.16)

project(LabDeviceReservation
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(labapp
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
)

target_include_directories(labapp PRIVATE src)

target_link_libraries(labapp PRIVATE Qt6::Widgets)
