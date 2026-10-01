cmake_minimum_required(VERSION 3.16)

project(LabDeviceReservation
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# 核心层：不依赖 Qt，可脱离 GUI 单独编译与测试
add_library(labcore INTERFACE)
target_include_directories(labcore INTERFACE src)

# GUI 层
qt_add_executable(labapp
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/DevicePage.cpp
    src/gui/DevicePage.h
)
target_link_libraries(labapp PRIVATE labcore Qt6::Widgets)

# 测试：仅依赖核心层
add_executable(test_model tests/test_model.cpp)
target_link_libraries(test_model PRIVATE labcore)

add_executable(test_datastruct tests/test_datastruct.cpp)
target_link_libraries(test_datastruct PRIVATE labcore)

add_executable(test_service tests/test_service.cpp)
target_link_libraries(test_service PRIVATE labcore)

add_executable(test_lab_service tests/test_lab_service.cpp)
target_link_libraries(test_lab_service PRIVATE labcore)
