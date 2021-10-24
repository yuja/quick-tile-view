find_package(Qt6 COMPONENTS Core Quick REQUIRED)

qt_add_qml_module(quick-tiler
  URI MyTile
  VERSION 1.0
  SOURCES
    flextilelayouter.cpp
    flextilelayouter.h
    flextiler.cpp
    flextiler.h
    tiler.cpp
    tiler.h
)

target_include_directories(quick-tiler INTERFACE .)
target_link_libraries(quick-tiler PUBLIC
  Qt6::Core
  Qt6::Quick
)
