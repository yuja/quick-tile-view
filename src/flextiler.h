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
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    QML_ATTACHED(FlexTilerAttached)
    QML_ELEMENT

public:
    explicit FlexTiler(QQuickItem *parent = nullptr);
    ~FlexTiler() override;

    static FlexTilerAttached *qmlAttachedProperties(QObject *object);

    QQmlComponent *delegate() { return tileDelegate_; }
    void setDelegate(QQmlComponent *delegate);

    int count() const;
    Q_INVOKABLE QQuickItem *itemAt(int index) const;

    Q_INVOKABLE void split(int index, Qt::Orientation orientation, int count = 2);

signals:
    void delegateChanged();
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
        UniqueItemPtr item; // may be nullptr
        std::unique_ptr<QQmlContext> context; // may be nullptr if !item
    };

    struct Vertex
    {
        int pixelPos;
        int tileIndex; // -1 if intermediate point or terminator
    };

    void recreateTiles();
    Tile createTile(const QRectF &normRect, int index);
    std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>> createTileItem(int index);
    void accumulateTiles();
    void resizeTiles();

    std::vector<Tile> tiles_;
    std::map<int, std::vector<Vertex>> horizontalVertices_; // x: {y}, updated by accumulateTiles()
    std::map<int, std::vector<Vertex>> verticalVertices_; // y: {x}, updated by accumulateTiles()
    QPointer<QQmlComponent> tileDelegate_ = nullptr;
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
