import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import MyTile

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    RowLayout {
        anchors.fill: parent

        TileGenerator {
            id: tileGenerator

            Layout.fillWidth: true
            Layout.fillHeight: true
            tile: Rectangle {
                id: tile

                // TODO
                border.color: "cyan"
                border.width: 1
                color: "#4000ffff"

                Label {
                    padding: 2
                    color: "white"
                    text: tile.TileGenerator.index
                    background: Rectangle {
                        color: "darkblue"
                    }
                }
            }
        }

        Pane {
            palette.window: "#eeeeee"
            Layout.fillHeight: true

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top

                RowLayout {
                    Label { text: "tile" }
                    SpinBox {
                        id: tileIndexSpinBox
                        Layout.preferredWidth: 80
                        editable: true
                        from: 0
                        to: 100  // TODO
                    }
                }

                Button {
                    text: "Split H"
                    onClicked: {
                        tileGenerator.split(tileIndexSpinBox.value, Qt.Horizontal);
                    }
                }

                Button {
                    text: "Split V"
                    onClicked: {
                        tileGenerator.split(tileIndexSpinBox.value, Qt.Vertical);
                    }
                }
            }
        }
    }
}
