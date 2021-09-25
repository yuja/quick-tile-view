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

    // TODO: migrate to handle component
    Item {
        id: hHandle
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: root.x > 0
        width: 5

        HoverHandler {
            cursorShape: Qt.SplitHCursor
        }

        DragHandler {
            target: null
            xAxis.enabled: true
            yAxis.enabled: false
            onCentroidChanged: {
                if (!active)
                    return;
                let tiler = root.Tiler.tiler;
                let pos = hHandle.mapToItem(
                        tiler, centroid.position.x - centroid.pressPosition.x, 0).x;
                tiler.moveTopLeftEdge(root.Tiler.index, Qt.Horizontal, pos);
            }
        }
    }

    // TODO: migrate to handle component
    Item {
        id: vHandle
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        visible: root.y > 0
        height: 5

        HoverHandler {
            cursorShape: Qt.SplitVCursor
        }

        DragHandler {
            target: null
            xAxis.enabled: false
            yAxis.enabled: true
            onCentroidChanged: {
                if (!active)
                    return;
                let tiler = root.Tiler.tiler;
                let pos = vHandle.mapToItem(
                        tiler, 0, centroid.position.y - centroid.pressPosition.y).y;
                tiler.moveTopLeftEdge(root.Tiler.index, Qt.Vertical, pos);
            }
        }
    }

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
