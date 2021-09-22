#pragma once
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <memory>
#include <tuple>
#include <vector>

class TilerAttached;

class Tiler : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *tile READ tile WRITE setTile NOTIFY tileChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    QML_ATTACHED(TilerAttached)
    QML_ELEMENT

public:
    explicit Tiler(QQuickItem *parent = nullptr);
    ~Tiler() override;

    static TilerAttached *qmlAttachedProperties(QObject *object);

    QQmlComponent *tile() { return tileComponent_; }
    void setTile(QQmlComponent *tile);

    int count() const;
    Q_INVOKABLE QQuickItem *itemAt(int tileIndex) const;

    Q_INVOKABLE void split(int tileIndex, Qt::Orientation orientation);
    Q_INVOKABLE void moveTopLeftEdge(int tileIndex, Qt::Orientation orientation, qreal itemPos);

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

signals:
    void tileChanged();
    void countChanged();

private:
    struct Tile
    {
        std::unique_ptr<QQuickItem> item; // may be nullptr
        std::unique_ptr<QQmlContext> context; // may be nullptr if !item
    };

    struct Band
    {
        int index; // >=0: tile[i], <0: splitMap[-i]
        qreal position;
    };

    struct Split
    {
        Qt::Orientation orientation;
        std::vector<Band> bands;
        QRectF outerRect; // cache updated by resizeTiles()
    };

    void recreateTiles();
    Tile createTile();
    std::tuple<int, int> findSplitBandByIndex(int index) const;
    std::tuple<int, int> findSplitBandByIndex(int index, Qt::Orientation orientation) const;
    void resizeTiles(int splitIndex, const QRectF &outerRect, int depth);

    std::vector<Tile> tiles_;
    std::vector<Split> splitMap_;
    QPointer<QQmlComponent> tileComponent_ = nullptr;
};

class TilerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)

public:
    explicit TilerAttached(QObject *parent = nullptr);

    int index() const { return index_; }
    void setIndex(int index);

signals:
    void indexChanged();

private:
    int index_ = -1;
};
