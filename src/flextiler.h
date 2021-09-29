#pragma once
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

class FlexTilerAttached;

class FlexTiler : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(QQmlComponent *horizontalHandle READ horizontalHandle WRITE setHorizontalHandle
                       NOTIFY horizontalHandleChanged FINAL)
    Q_PROPERTY(QQmlComponent *verticalHandle READ verticalHandle WRITE setVerticalHandle NOTIFY
                       verticalHandleChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    QML_ATTACHED(FlexTilerAttached)
    QML_ELEMENT

public:
    explicit FlexTiler(QQuickItem *parent = nullptr);
    ~FlexTiler() override;

    static FlexTilerAttached *qmlAttachedProperties(QObject *object);

    QQmlComponent *delegate() { return tileDelegate_; }
    void setDelegate(QQmlComponent *delegate);

    QQmlComponent *horizontalHandle() { return horizontalHandle_; }
    void setHorizontalHandle(QQmlComponent *handle);

    QQmlComponent *verticalHandle() { return verticalHandle_; }
    void setVerticalHandle(QQmlComponent *handle);

    int count() const;
    Q_INVOKABLE QQuickItem *itemAt(int index) const;

    Q_INVOKABLE void split(int index, Qt::Orientation orientation, int count = 2);

signals:
    void delegateChanged();
    void horizontalHandleChanged();
    void verticalHandleChanged();
    void countChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void updatePolish() override;

private:
    class ItemDeleter
    {
    public:
        void operator()(QQuickItem *item) const;
    };

    using UniqueItemPtr = std::unique_ptr<QQuickItem, ItemDeleter>;

    struct Tile
    {
        QRectF normRect;
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
        int pixelPos;
        int tileIndex; // -1 if terminator
        int handleSpan; // 0: invisible, >0: span n vertices
        bool primary; // is starting vertex in orthogonal axis?
    };

    void recreateTiles();
    Tile createTile(const QRectF &normRect, int index);
    std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>> createTileItem(int index);
    std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>>
    createHandleItem(Qt::Orientation orientation);
    QRectF extendedOuterRect() const;
    void accumulateTiles();
    void resizeTiles();

    std::vector<Tile> tiles_;
    std::map<int, std::vector<Vertex>> horizontalVertices_; // x: {y}, updated by accumulateTiles()
    std::map<int, std::vector<Vertex>> verticalVertices_; // y: {x}, updated by accumulateTiles()
    QPointer<QQmlComponent> tileDelegate_ = nullptr;
    QPointer<QQmlComponent> horizontalHandle_ = nullptr;
    QPointer<QQmlComponent> verticalHandle_ = nullptr;
    qreal horizontalHandleWidth_ = 0.0;
    qreal verticalHandleHeight_ = 0.0;
};

class FlexTilerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FlexTiler *tiler READ tiler NOTIFY tilerChanged FINAL)
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)

public:
    explicit FlexTilerAttached(QObject *parent = nullptr);

    FlexTiler *tiler() const { return tiler_; }
    void setTiler(FlexTiler *tiler);

    int index() const { return index_; }
    void setIndex(int index);

signals:
    void tilerChanged();
    void indexChanged();

private:
    QPointer<FlexTiler> tiler_ = nullptr;
    int index_ = -1;
};
