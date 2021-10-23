import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import MyTile

Window {
    id: root

    property alias targetTileIndex: tileIndexSpinBox.value
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
                FlexTiler.minimumWidth: minimumWidth
                FlexTiler.minimumHeight: minimumHeight
                highlighted: this === flexTiler.currentItem
                index: FlexTiler.index
                closable: FlexTiler.closable
                onCloseRequested: {
                    flexTiler.close(index);
                }
                onTapped: {
                    flexTiler.currentIndex = FlexTiler.index;
                    root.targetTileIndex = FlexTiler.index;
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
            property int currentIndex: 0
            readonly property Item currentItem: currentIndex >= 0 ? itemAt(currentIndex) : null
            delegate: Tile {
                Tiler.minimumWidth: minimumWidth
                Tiler.minimumHeight: minimumHeight
                highlighted: Tiler.index === tiler.currentIndex
                index: Tiler.index
                closable: tiler.count > 1
                onCloseRequested: {
                    tiler.close(index);
                }
                onTapped: {
                    tiler.currentIndex = Tiler.index;
                    root.targetTileIndex = Tiler.index;
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

                    Label { text: "current" }
                    Label {
                        text: root.tiler.currentIndex
                    }

                    Label { text: "target" }
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
                        value: root.tiler.itemAt(root.targetTileIndex).minimumWidth
                        onValueModified: {
                            root.tiler.itemAt(root.targetTileIndex).minimumWidth = value;
                        }
                    }

                    Label { text: "min h" }
                    SpinBox {
                        Layout.preferredWidth: 80
                        editable: true
                        from: 0
                        to: 1000
                        value: root.tiler.itemAt(root.targetTileIndex).minimumHeight
                        onValueModified: {
                            root.tiler.itemAt(root.targetTileIndex).minimumHeight = value;
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
                        to: 100
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
