import QtQuick
import QtQuick.Controls
import MyTile

Rectangle {
    id: root

    property bool highlighted
    readonly property bool occupied: children.length > 3

    // TODO
    border.color: highlighted ? "cyan" : "lightgray"
    border.width: 1
    color: highlighted ? "#4000ffff" : "#40808080"

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
                let pos = vHandle.mapToItem(
                        tiler, 0, centroid.position.y - centroid.pressPosition.y).y;
                tiler.moveTopLeftEdge(root.Tiler.index, Qt.Vertical, pos);
            }
        }
    }

    Label {
        padding: 2
        color: "white"
        text: root.Tiler.index
        background: Rectangle {
            color: "darkblue"
        }
    }
}
