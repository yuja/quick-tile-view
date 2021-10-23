import QtQuick

Rectangle {
    implicitWidth: 5
    color: "gray"
    border.color: "black"
    border.width: 1

    HoverHandler {
        cursorShape: Qt.SplitHCursor
    }
}
