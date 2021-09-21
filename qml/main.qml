import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import MyTile

Window {
    id: root

    property alias currentTileIndex: tileIndexSpinBox.value

    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    RowLayout {
        anchors.fill: parent

        Tiler {
            id: tiler

            Layout.fillWidth: true
            Layout.fillHeight: true
            tile: Rectangle {
                id: tile

                readonly property bool highlighted: Tiler.index === root.currentTileIndex
                readonly property bool occupied: children.length > 1

                // TODO
                border.color: highlighted ? "cyan" : "lightgray"
                border.width: 1
                color: highlighted ? "#4000ffff" : "#40808080"

                Label {
                    padding: 2
                    color: "white"
                    text: tile.Tiler.index
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
                        to: tiler.count - 1
                    }
                }

                Button {
                    text: "Split H"
                    onClicked: {
                        tiler.split(tileIndexSpinBox.value, Qt.Horizontal);
                    }
                }

                Button {
                    text: "Split V"
                    onClicked: {
                        tiler.split(tileIndexSpinBox.value, Qt.Vertical);
                    }
                }

                Button {
                    text: "Add Frame"
                    onClicked: {
                        dummyFrameListModel.append({
                            name: "Frame %1".arg(dummyFrameListModel.count)});
                    }
                }
            }
        }
    }

    Repeater {
        model: ListModel {
            id: dummyFrameListModel
        }

        onItemAdded: function(index, item) {
            for (let i = 0; i < tiler.count; ++i) {
                let t = tiler.itemAt(i);
                if (t.occupied)
                    continue;
                item.parent = t;
                item.visible = true;
                break;
            }
        }

        Rectangle {
            id: frame

            required property string name

            anchors.fill: parent
            color: "#400000ff"
            visible: false

            Label {
                anchors.centerIn: parent
                text: frame.name
            }
        }
    }
}
