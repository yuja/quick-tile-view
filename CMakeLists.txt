cmake_minimum_required(VERSION 3.14)

project(quick-tile-view VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

find_package(Qt6 COMPONENTS Core Quick REQUIRED)

add_subdirectory(src)

qt_add_executable(quick-tile-view
  src/main.cpp
  qml.qrc
)

target_compile_definitions(quick-tile-view PRIVATE
  $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(quick-tile-view PRIVATE
  Qt6::Core
  Qt6::Quick
)
