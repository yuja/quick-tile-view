#include <QPointF>
#include <QtQml>
#include <algorithm>
#include "tiler.h"

namespace {
QRectF itemRect(const QQuickItem *item)
{
    return { QPointF(0.0, 0.0), item->size() };
}

TilerAttached *tileAttached(QQuickItem *item)
{
    if (!item)
        return nullptr;
    return qobject_cast<TilerAttached *>(qmlAttachedPropertiesObject<Tiler>(item));
}
}

Tiler::Tiler(QQuickItem *parent) : QQuickItem(parent)
{
    tiles_.push_back({ nullptr, nullptr });
    splitMap_.push_back({ Qt::Horizontal, { { 0, 0.0 } }, {} });
}

TilerAttached *Tiler::qmlAttachedProperties(QObject *object)
{
    return new TilerAttached(object);
}

Tiler::~Tiler() = default;

void Tiler::setTile(QQmlComponent *tile)
{
    if (tileComponent_ == tile)
        return;

    tileComponent_ = tile;
    recreateTiles();
    emit tileChanged();
}

void Tiler::recreateTiles()
{
    std::vector<Tile> newTiles;
    newTiles.reserve(tiles_.size());
    for (size_t i = 0; i < tiles_.size(); ++i) {
        auto t = createTile();
        if (auto *a = tileAttached(t.item.get())) {
            a->setIndex(static_cast<int>(i));
        }
        newTiles.push_back(std::move(t));
    }
    tiles_.swap(newTiles);
    resizeTiles(0, itemRect(this), 0);
    // Old tile items should be destroyed after new tiles get created.
}

auto Tiler::createTile() -> Tile
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

int Tiler::count() const
{
    return static_cast<int>(tiles_.size());
}

QQuickItem *Tiler::itemAt(int tileIndex) const
{
    if (tileIndex < 0 || tileIndex >= static_cast<int>(tiles_.size())) {
        qmlWarning(this) << "tile index out of range:" << tileIndex;
        return nullptr;
    }

    return tiles_.at(static_cast<size_t>(tileIndex)).item.get();
}

/// Finds indices of (split, band) for the given tile (>= 0) or split (< 0) index.
std::tuple<int, int> Tiler::findSplitBandByIndex(int index) const
{
    for (size_t i = 0; i < splitMap_.size(); ++i) {
        const auto &split = splitMap_.at(i);
        const auto p = std::find_if(split.bands.begin(), split.bands.end(),
                                    [index](const auto &b) { return b.index == index; });
        if (p == split.bands.end())
            continue;
        return { static_cast<int>(i), static_cast<int>(p - split.bands.begin()) };
    }
    return { -1, -1 };
}

void Tiler::split(int tileIndex, Qt::Orientation orientation)
{
    if (tileIndex < 0 || tileIndex >= static_cast<int>(tiles_.size())) {
        qmlWarning(this) << "tile index out of range:" << tileIndex;
        return;
    }

    // Insert new tile and adjust indices.
    tiles_.insert(tiles_.begin() + tileIndex + 1, createTile());
    for (size_t i = static_cast<size_t>(tileIndex) + 1; i < tiles_.size(); ++i) {
        if (auto *a = tileAttached(tiles_.at(i).item.get())) {
            a->setIndex(static_cast<int>(i));
        }
    }
    for (auto &split : splitMap_) {
        for (auto &b : split.bands) {
            if (b.index <= tileIndex)
                continue;
            b.index += 1;
        }
    }

    // Allocate band for the new tile.
    const auto [splitIndex, bandIndex] = findSplitBandByIndex(tileIndex);
    Q_ASSERT(splitIndex >= 0 && bandIndex >= 0);
    auto &split = splitMap_.at(static_cast<size_t>(splitIndex));
    if (split.orientation == orientation || split.bands.size() <= 1) {
        const auto p = split.bands.begin() + bandIndex;
        const auto q = std::next(p);
        const qreal size = (q != split.bands.end() ? q->position : 1.0) - p->position;
        split.orientation = orientation;
        split.bands.insert(std::next(p), { tileIndex + 1, p->position + size / 2 });
        // p and q may be invalidated.
    } else {
        auto &b = split.bands.at(static_cast<size_t>(bandIndex));
        b.index = -static_cast<int>(splitMap_.size());
        splitMap_.push_back({ orientation, { { tileIndex, 0.0 }, { tileIndex + 1, 0.5 } }, {} });
        // split and b may be invalidated.
    }

    resizeTiles(0, QRectF(QPointF(0.0, 0.0), size()), 0); // TODO: optimize
    emit countChanged();
}

void Tiler::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    resizeTiles(0, newGeometry, 0);
}

void Tiler::resizeTiles(int splitIndex, const QRectF &outerRect, int depth)
{
    Q_ASSERT_X(depth < static_cast<int>(splitMap_.size()), __FUNCTION__, "bad recursion detected");
    auto &split = splitMap_.at(static_cast<size_t>(splitIndex));
    split.outerRect = outerRect;
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

TilerAttached::TilerAttached(QObject *parent) : QObject(parent) { }

void TilerAttached::setIndex(int index)
{
    if (index_ == index)
        return;
    index_ = index;
    emit indexChanged();
}
