#include <QPointF>
#include <QtQml>
#include "tilegenerator.h"

namespace {
QRectF itemRect(const QQuickItem *item)
{
    return { QPointF(0.0, 0.0), item->size() };
}
}

TileGenerator::TileGenerator(QQuickItem *parent) : QQuickItem(parent)
{
    tiles_.push_back({ nullptr, nullptr });
    splitMap_.push_back({ Qt::Horizontal, { { 0, 0.0 } } });
}

TileGenerator::~TileGenerator() = default;

void TileGenerator::setTile(QQmlComponent *tile)
{
    if (tileComponent_ == tile)
        return;

    tileComponent_ = tile;
    recreateTiles();
    emit tileChanged();
}

void TileGenerator::recreateTiles()
{
    std::vector<Tile> newTiles;
    newTiles.reserve(tiles_.size());
    for (size_t i = 0; i < tiles_.size(); ++i) {
        newTiles.push_back(createTile());
    }
    tiles_.swap(newTiles);
    resizeTiles(0, itemRect(this), 0);
    // Old tile items should be destroyed after new tiles get created.
}

auto TileGenerator::createTile() -> Tile
{
    if (!tileComponent_)
        return {};

    // See qquicksplitview.cpp
    auto *creationContext = tileComponent_->creationContext();
    if (!creationContext)
        creationContext = qmlContext(this);
    auto context = std::make_unique<QQmlContext>(creationContext);
    context->setContextObject(this);

    auto *obj = tileComponent_->create(context.get());
    if (auto *item = qobject_cast<QQuickItem *>(obj)) {
        item->setParentItem(this);
        return { std::unique_ptr<QQuickItem>(item), std::move(context) };
    } else {
        qmlWarning(this) << "tile component does not create an item";
        delete obj;
        return {};
    }
}

void TileGenerator::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    resizeTiles(0, newGeometry, 0);
}

void TileGenerator::resizeTiles(int splitIndex, const QRectF &outerRect, int depth)
{
    Q_ASSERT_X(depth < static_cast<int>(splitMap_.size()), __FUNCTION__, "bad recursion detected");
    const auto &split = splitMap_.at(static_cast<size_t>(splitIndex));
    for (size_t i = 0; i < split.bands.size(); ++i) {
        const auto &band = split.bands.at(i);
        const qreal endPos = i + 1 < split.bands.size() ? split.bands.at(i + 1).position : 1.0;
        const auto rect = split.orientation == Qt::Horizontal
                ? QRectF(outerRect.x() + band.position * outerRect.width(), outerRect.y(),
                         (endPos - band.position) * outerRect.width(), outerRect.height())
                : QRectF(outerRect.x(), outerRect.y() + band.position * outerRect.height(),
                         outerRect.width(), (endPos - band.position) * outerRect.height());
        if (band.index >= 0) {
            auto &item = tiles_.at(static_cast<size_t>(band.index)).item;
            if (!item)
                continue;
            item->setPosition(rect.topLeft());
            item->setSize(rect.size());
        } else {
            resizeTiles(-band.index, rect, depth + 1);
        }
    }
}
