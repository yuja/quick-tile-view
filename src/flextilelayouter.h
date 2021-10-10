#pragma once
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <QSizeF>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

class FlexTileLayouter : public QQuickItem
{
    Q_OBJECT

public:
    explicit FlexTileLayouter(QQuickItem *parent = nullptr);
    ~FlexTileLayouter() override;

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

    struct AdjacentIndices
    {
        std::vector<int> left;
        std::vector<int> right;
        std::vector<int> top;
        std::vector<int> bottom;
    };

    size_t count() const { return tiles_.size(); }
    const Tile &tileAt(size_t index) const { return tiles_.at(index); }
    Tile &tileAt(size_t index) { return tiles_.at(index); }
    std::tuple<int, Qt::Orientations> findTileByHandleItem(const QQuickItem *item) const;

    void split(size_t index, Qt::Orientation orientation, std::vector<Tile> &&newTiles,
               const QSizeF &snapSize);
    Q_INVOKABLE void close(int index);
    void resetMovingState();
    AdjacentIndices collectAdjacentTiles(int index, Qt::Orientations orientations) const;
    QRectF calculateMovableNormRect(int index, const AdjacentIndices &adjacentIndices) const;
    void moveAdjacentTiles(const AdjacentIndices &indices, const QPointF &normPos);
    QRectF extendedOuterPixelRect() const;
    void accumulateTiles();
    void resizeTiles();

signals:
    void countChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void updatePolish() override;

private:
    std::vector<Tile> tiles_;
    VerticesMap xyVerticesMap_; // x: {y: v}, updated by accumulateTiles()
    VerticesMap yxVerticesMap_; // y: {x: v}, updated by accumulateTiles()
    qreal horizontalHandlePixelWidth_ = 0.0;
    qreal verticalHandlePixelHeight_ = 0.0;
    AdjacentIndices movingTiles_;
    QRectF movableNormRect_;
    QPointF movingHandleGrabPixelOffset_;
    VerticesMap preMoveXyVerticesMap_;
    VerticesMap preMoveYxVerticesMap_;
};
