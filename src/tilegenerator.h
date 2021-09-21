#ifndef TILEGENERATOR_H
#define TILEGENERATOR_H

#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <memory>
#include <vector>

class TileGeneratorAttached;

class TileGenerator : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *tile READ tile WRITE setTile NOTIFY tileChanged FINAL)
    QML_ATTACHED(TileGeneratorAttached)
    QML_ELEMENT

public:
    explicit TileGenerator(QQuickItem *parent = nullptr);
    ~TileGenerator() override;

    static TileGeneratorAttached *qmlAttachedProperties(QObject *object);

    QQmlComponent *tile() { return tileComponent_; }
    void setTile(QQmlComponent *tile);

    Q_INVOKABLE void split(int tileIndex, Qt::Orientation orientation);

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

signals:
    void tileChanged();

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
    };

    void recreateTiles();
    Tile createTile();
    void resizeTiles(int splitIndex, const QRectF &outerRect, int depth);

    std::vector<Tile> tiles_;
    std::vector<Split> splitMap_;
    QPointer<QQmlComponent> tileComponent_ = nullptr;
};

class TileGeneratorAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)

public:
    explicit TileGeneratorAttached(QObject *parent = nullptr);

    int index() const { return index_; }
    void setIndex(int index);

signals:
    void indexChanged();

private:
    int index_ = -1;
};

#endif // TILEGENERATOR_H
