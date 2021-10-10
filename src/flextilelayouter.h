#pragma once
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
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

    QQmlComponent *delegate() { return tileDelegate_; }
    void setDelegate(QQmlComponent *delegate);

    QQmlComponent *horizontalHandle() { return horizontalHandle_; }
    void setHorizontalHandle(QQmlComponent *handle);

    QQmlComponent *verticalHandle() { return verticalHandle_; }
    void setVerticalHandle(QQmlComponent *handle);

    int count() const;
    Q_INVOKABLE QQuickItem *itemAt(int index) const;
    std::tuple<int, Qt::Orientations> findTileByHandleItem(const QQuickItem *item) const;

    Q_INVOKABLE void split(int index, Qt::Orientation orientation, int count = 2);
    Q_INVOKABLE void close(int index);
    void resetMovingState();
    AdjacentIndices collectAdjacentTiles(int index, Qt::Orientations orientations) const;
    QRectF calculateMovableNormRect(int index, const AdjacentIndices &adjacentIndices) const;
    void moveAdjacentTiles(const AdjacentIndices &indices, const QPointF &normPos);
    QRectF extendedOuterPixelRect() const;
    void accumulateTiles();
    void resizeTiles();

signals:
    void delegateChanged();
    void horizontalHandleChanged();
    void verticalHandleChanged();
    void countChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void updatePolish() override;

private:
    void recreateTiles();
    Tile createTile(const KeyRect &normRect, int index);
    std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>> createTileItem(int index);
    std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>>
    createHandleItem(Qt::Orientation orientation);

    std::vector<Tile> tiles_;
    VerticesMap xyVerticesMap_; // x: {y: v}, updated by accumulateTiles()
    VerticesMap yxVerticesMap_; // y: {x: v}, updated by accumulateTiles()
    QPointer<QQmlComponent> tileDelegate_ = nullptr;
    QPointer<QQmlComponent> horizontalHandle_ = nullptr;
    QPointer<QQmlComponent> verticalHandle_ = nullptr;
    qreal horizontalHandlePixelWidth_ = 0.0;
    qreal verticalHandlePixelHeight_ = 0.0;
    AdjacentIndices movingTiles_;
    QRectF movableNormRect_;
    QPointF movingHandleGrabPixelOffset_;
    VerticesMap preMoveXyVerticesMap_;
    VerticesMap preMoveYxVerticesMap_;
};
