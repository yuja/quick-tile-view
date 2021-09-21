import QtQuick
import QtQuick.Controls
import MyTile

Container {
    function split(tileIndex, orientation) {
        tileGenerator.split(tileIndex, orientation)
    }

    contentItem: TileGenerator {
        id: tileGenerator
        tile: Rectangle {
            // TODO
            border.color: "cyan"
            border.width: 1
            color: "#4000ffff"
        }
    }
}
