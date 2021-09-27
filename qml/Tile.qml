import QtQuick
import QtQuick.Controls
import MyTile

Item {
    id: root

    property bool highlighted
    property int index
    property alias contentItem: contentItem
    property alias closable: closeButton.enabled
    readonly property bool occupied: contentItem.children.length > 0

    signal closeRequested()
    signal tapped()

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
        text: root.index
        background: Rectangle {
            color: "darkblue"
        }
    }

    Button {
        id: closeButton
        anchors.right: contentItem.right
        anchors.top: contentItem.top
        width: 20
        height: 20
        text: "x"
        onClicked: root.closeRequested()
    }
}
