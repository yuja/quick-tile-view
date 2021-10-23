cmake_minimum_required(VERSION 3.14)

project(quick-tile-view VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)
# Help Qt Creator find the custom module
# https://www.qt.io/blog/qml-modules-in-qt-6.2
set(QML_IMPORT_PATH ${CMAKE_CURRENT_BINARY_DIR}/qml CACHE STRING "" FORCE)

add_subdirectory(examples/tiling)
add_subdirectory(src)
