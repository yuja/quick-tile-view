import QtQuick
import QtQuick.Controls
import MyTile

Item {
    id: root

    property bool highlighted
    property alias contentItem: contentItem
    readonly property bool occupied: contentItem.children.length > 0

    Tiler.minimumWidth: 10
    Tiler.minimumHeight: 20

    Rectangle {
        id: hHandle
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: root.x > 0
        width: 5
        color: "gray"

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

    Rectangle {
        id: vHandle
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        visible: root.y > 0
        height: 5
        color: "gray"

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
        anchors.left: hHandle.right
        anchors.top: vHandle.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        // TODO
        border.color: root.highlighted ? "cyan" : "lightgray"
        border.width: 1
        color: root.highlighted ? "#4000ffff" : "#40808080"
    }

    Label {
        anchors.left: hHandle.right
        anchors.top: vHandle.bottom
        padding: 2
        color: "white"
        text: root.Tiler.index
        background: Rectangle {
            color: "darkblue"
        }
    }
}
