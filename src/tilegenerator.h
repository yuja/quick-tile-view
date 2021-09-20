#ifndef TILEGENERATOR_H
#define TILEGENERATOR_H

#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <memory>
#include <vector>

class TileGenerator : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *tile READ tile WRITE setTile NOTIFY tileChanged FINAL)
    QML_ELEMENT

public:
    explicit TileGenerator(QQuickItem *parent = nullptr);
    ~TileGenerator() override;

    QQmlComponent *tile() { return tileComponent_; }
    void setTile(QQmlComponent *tile);

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

#endif // TILEGENERATOR_H
