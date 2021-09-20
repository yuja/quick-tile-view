import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    RowLayout {
        anchors.fill: parent

        TileView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Pane {
            palette.window: "#eeeeee"
            Layout.fillHeight: true
        }
    }
}
