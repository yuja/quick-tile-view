#pragma once
#include <QPoint>
#include <QPointF>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <QSizeF>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

class FlexTileLayouter
{
public:
    FlexTileLayouter();
    FlexTileLayouter(const FlexTileLayouter &) = delete;
    void operator=(const FlexTileLayouter &) = delete;
    ~FlexTileLayouter();

    class ItemDeleter
    {
    public:
        void operator()(QQuickItem *item) const;
    };

    using UniqueItemPtr = std::unique_ptr<QQuickItem, ItemDeleter>;

    struct KeyRect
    {
        qreal x0, y0;
        qreal x1, y1;
    };

    struct Tile
    {
        // These points can be used as map keys.
        KeyRect normRect;
        // Item and context may be nullptr if the corresponding component is unspecified
        // or invalid.
        UniqueItemPtr item;
        std::unique_ptr<QQmlContext> context;
        // Not all tiles need horizontal/vertical handles, but handle items are created
        // per tile to support the maximum possibility. Unused handles are just hidden.
        UniqueItemPtr horizontalHandleItem;
        std::unique_ptr<QQmlContext> horizontalHandleContext;
        UniqueItemPtr verticalHandleItem;
        std::unique_ptr<QQmlContext> verticalHandleContext;
    };

    struct Vertex
    {
        int tileIndex; // -1 if terminator
        qreal normHandleSize; // 0: invisible, >0: span n pixels
        bool primary; // is starting vertex in orthogonal axis?
        bool collapsible; // can any of the adjacent tiles be expanded to fill this cell?
    };

    using VerticesMap = std::map<qreal, std::map<qreal, Vertex>>; // x: {y: v} or y: {x: v}

    size_t count() const { return tiles_.size(); }
    const Tile &tileAt(size_t index) const { return tiles_.at(index); }
    Tile &tileAt(size_t index) { return tiles_.at(index); }
    std::tuple<int, Qt::Orientations> findTileByHandleItem(const QQuickItem *item) const;

    void split(size_t index, Qt::Orientation orientation, std::vector<Tile> &&newTiles,
               const QSizeF &snapSize);
    bool close(size_t index);

    bool isMoving() const;
    void startMoving(size_t index, Qt::Orientations orientations, const QRectF &outerPixelRect,
                     const QSizeF &handlePixelSize);
    void moveTo(const QPointF &normPos, const QSizeF &snapSize);
    void resetMovingState();

    void resizeTiles(const QRectF &outerPixelRect, const QSizeF &handlePixelSize);

private:
    struct AdjacentIndices
    {
        std::vector<int> left;
        std::vector<int> right;
        std::vector<int> top;
        std::vector<int> bottom;
    };

    AdjacentIndices collectAdjacentTiles(size_t index, Qt::Orientations orientations) const;
    QRectF calculateMovableNormRect(size_t index, const AdjacentIndices &adjacentIndices,
                                    const QRectF &outerPixelRect,
                                    const QSizeF &handlePixelSize) const;
    void moveAdjacentTiles(const AdjacentIndices &indices, const QPointF &normPos);
    void invalidateVerticesMap();
    void ensureVerticesMapBuilt();

    std::vector<Tile> tiles_;
    VerticesMap xyVerticesMap_; // x: {y: v}, updated by ensureVerticesMapBuilt()
    VerticesMap yxVerticesMap_; // y: {x: v}, updated by ensureVerticesMapBuilt()
    AdjacentIndices movingTiles_;
    QRectF movableNormRect_;
    VerticesMap preMoveXyVerticesMap_;
    VerticesMap preMoveYxVerticesMap_;
};
