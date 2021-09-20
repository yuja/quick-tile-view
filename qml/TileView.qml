import QtQuick
import QtQuick.Controls
import MyTile

Container {
    contentItem: TileGenerator {
        tile: Rectangle {
            // TODO
            border.color: "cyan"
            border.width: 1
            color: "#4000ffff"
        }
    }
}
