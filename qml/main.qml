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
            delegate: Tile {
                Tiler.minimumWidth: 5
                Tiler.minimumHeight: 15
                highlighted: Tiler.index === root.currentTileIndex
                index: Tiler.index
                closable: tiler.count > 1
                onCloseRequested: {
                    tiler.close(index);
                }
                onTapped: {
                    root.currentTileIndex = Tiler.index;
                }
            }
            horizontalHandle: Rectangle {
                implicitWidth: 5
                z: 1
                color: "gray"
                HoverHandler {
                    cursorShape: Qt.SplitHCursor
                }
            }
            verticalHandle: Rectangle {
                implicitHeight: 5
                z: 1
                color: "gray"
                HoverHandler {
                    cursorShape: Qt.SplitVCursor
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

                GridLayout {
                    columns: 2

                    Label { text: "tile" }
                    SpinBox {
                        id: tileIndexSpinBox
                        Layout.preferredWidth: 80
                        editable: true
                        from: 0
                        to: tiler.count - 1
                    }

                    Label { text: "min w" }
                    SpinBox {
                        Layout.preferredWidth: 80
                        editable: true
                        from: 0
                        to: 1000
                        value: tiler.itemAt(root.currentTileIndex).Tiler.minimumWidth
                        onValueModified: {
                            tiler.itemAt(root.currentTileIndex).Tiler.minimumWidth = value;
                        }
                    }

                    Label { text: "min h" }
                    SpinBox {
                        Layout.preferredWidth: 80
                        editable: true
                        from: 0
                        to: 1000
                        value: tiler.itemAt(root.currentTileIndex).Tiler.minimumHeight
                        onValueModified: {
                            tiler.itemAt(root.currentTileIndex).Tiler.minimumHeight = value;
                        }
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
                item.parent = t.contentItem;
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
