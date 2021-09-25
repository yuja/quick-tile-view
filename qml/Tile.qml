import QtQuick
import QtQuick.Controls
import MyTile

Item {
    id: root

    property bool highlighted
    property alias contentItem: contentItem
    readonly property bool occupied: contentItem.children.length > 0
    signal tapped()

    Tiler.minimumWidth: 5
    Tiler.minimumHeight: 15

    Rectangle {
        id: contentItem
        anchors.fill: parent
        // TODO
        border.color: root.highlighted ? "cyan" : "lightgray"
        border.width: 1
        color: root.highlighted ? "#4000ffff" : "#40808080"

        TapHandler {
            onTapped: root.tapped()
        }
    }

    Label {
        anchors.left: contentItem.left
        anchors.top: contentItem.top
        padding: 2
        color: "white"
        text: root.Tiler.index
        background: Rectangle {
            color: "darkblue"
        }
    }

    Button {
        anchors.right: contentItem.right
        anchors.top: contentItem.top
        width: 20
        height: 20
        text: "x"
        enabled: root.Tiler.tiler.count > 1
        onClicked: {
            root.Tiler.tiler.close(root.Tiler.index);
        }
    }
}
