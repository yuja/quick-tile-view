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
            id: tile

            // TODO
            border.color: "cyan"
            border.width: 1
            color: "#4000ffff"

            Label {
                padding: 2
                color: "white"
                text: tile.TileGenerator.index
                background: Rectangle {
                    color: "darkblue"
                }
            }
        }
    }
}
