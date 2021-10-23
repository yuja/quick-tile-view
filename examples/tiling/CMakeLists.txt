find_package(Qt6 COMPONENTS Core Quick REQUIRED)

qt_add_executable(quick-tile-view
  main.cpp
)

qt_add_qml_module(quick-tile-view
  URI MyTilingExample
  VERSION 1.0
  QML_FILES
    HorizontalHandle.qml
    Tile.qml
    VerticalHandle.qml
    main.qml
)

target_compile_definitions(quick-tile-view PRIVATE
  $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(quick-tile-view PRIVATE
  Qt6::Core
  Qt6::Quick
)
