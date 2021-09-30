import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import MyTile

Window {
    id: root

    property alias currentTileIndex: tileIndexSpinBox.value
    property alias tiler: tilerLoader.item

    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Component {
        id: flexTilerComponent
        FlexTiler {
            id: flexTiler
            delegate: Tile {
                // TODO: Tiler.minimumWidth: minimumWidth
                // TODO: Tiler.minimumHeight: minimumHeight
                highlighted: FlexTiler.index === root.currentTileIndex
                index: FlexTiler.index
                closable: flexTiler.count > 1  // TODO: FlexTiler.closable ?
                onCloseRequested: {
                    flexTiler.close(index);
                }
                onTapped: {
                    root.currentTileIndex = FlexTiler.index;
                }
            }
            horizontalHandle: HorizontalHandle {
                z: 1
            }
            verticalHandle: VerticalHandle {
                z: 1
            }
        }
    }

    Component {
        id: treeTilerComponent
        Tiler {
            id: tiler
            delegate: Tile {
                Tiler.minimumWidth: minimumWidth
                Tiler.minimumHeight: minimumHeight
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
            horizontalHandle: HorizontalHandle {
                z: 1
            }
            verticalHandle: VerticalHandle {
                z: 1
            }
        }
    }

    RowLayout {
        anchors.fill: parent

        Loader {
            id: tilerLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: [flexTilerComponent, treeTilerComponent][tilerTypeComboBox.currentIndex]
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

                    Label { text: "type" }
                    ComboBox {
                        id: tilerTypeComboBox
                        Layout.preferredWidth: 80
                        model: ["flex", "tree"]
                    }

                    Label { text: "tile" }
                    SpinBox {
                        id: tileIndexSpinBox
                        Layout.preferredWidth: 80
                        editable: true
                        from: 0
                        to: root.tiler.count - 1
                    }

                    Label { text: "min w" }
                    SpinBox {
                        Layout.preferredWidth: 80
                        editable: true
                        from: 0
                        to: 1000
                        value: root.tiler.itemAt(root.currentTileIndex).minimumWidth
                        onValueModified: {
                            root.tiler.itemAt(root.currentTileIndex).minimumWidth = value;
                        }
                    }

                    Label { text: "min h" }
                    SpinBox {
                        Layout.preferredWidth: 80
                        editable: true
                        from: 0
                        to: 1000
                        value: root.tiler.itemAt(root.currentTileIndex).minimumHeight
                        onValueModified: {
                            root.tiler.itemAt(root.currentTileIndex).minimumHeight = value;
                        }
                    }
                }

                Button {
                    text: "Split H"
                    onClicked: {
                        if (splitCountSpinBox.enabled) {
                            root.tiler.split(tileIndexSpinBox.value, Qt.Horizontal,
                                             splitCountSpinBox.value);
                        } else {
                            root.tiler.split(tileIndexSpinBox.value, Qt.Horizontal);
                        }
                    }
                }

                Button {
                    text: "Split V"
                    onClicked: {
                        if (splitCountSpinBox.enabled) {
                            root.tiler.split(tileIndexSpinBox.value, Qt.Vertical,
                                             splitCountSpinBox.value);
                        } else {
                            root.tiler.split(tileIndexSpinBox.value, Qt.Vertical);
                        }
                    }
                }

                RowLayout {
                    Label { text: "n" }
                    SpinBox {
                        id: splitCountSpinBox
                        Layout.preferredWidth: 80
                        editable: true
                        enabled: tilerTypeComboBox.currentIndex === 0
                        from: 2
                        to: 10
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
            for (let i = 0; i < root.tiler.count; ++i) {
                let t = root.tiler.itemAt(i);
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
