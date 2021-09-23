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
    splitMap_.push_back({ Qt::Horizontal, { { 0, 0.0 } }, {}, {} });
}

TilerAttached *Tiler::qmlAttachedProperties(QObject *object)
{
    return new TilerAttached(object);
}

Tiler::~Tiler() = default;

void Tiler::setDelegate(QQmlComponent *delegate)
{
    if (tileDelegate_ == delegate)
        return;

    tileDelegate_ = delegate;
    recreateTiles();
    emit delegateChanged();
}

void Tiler::recreateTiles()
{
    std::vector<Tile> newTiles;
    newTiles.reserve(tiles_.size());
    for (size_t i = 0; i < tiles_.size(); ++i) {
        auto t = createTile();
        if (auto *a = tileAttached(t.item.get())) {
            a->setTiler(this);
            a->setIndex(static_cast<int>(i));
        }
        newTiles.push_back(std::move(t));
    }
    tiles_.swap(newTiles);
    polish();
    // Old tile items should be destroyed after new tiles get created.
}

auto Tiler::createTile() -> Tile
{
    if (!tileDelegate_)
        return {};

    // See qquicksplitview.cpp
    auto *creationContext = tileDelegate_->creationContext();
    if (!creationContext)
        creationContext = qmlContext(this);
    auto context = std::make_unique<QQmlContext>(creationContext);
    context->setContextObject(this);

    auto *obj = tileDelegate_->create(context.get());
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

std::tuple<int, int> Tiler::findSplitBandByIndex(int index, Qt::Orientation orientation) const
{
    const auto [splitIndex, bandIndex] = findSplitBandByIndex(index);
    if (splitIndex < 0)
        return { -1, -1 };
    const auto &split = splitMap_.at(static_cast<size_t>(splitIndex));
    if (split.orientation == orientation)
        return { splitIndex, bandIndex };
    if (splitIndex == 0)
        return { -1, -1 }; // no more outer split
    // outer split, if exists, should be the other orientation
    return findSplitBandByIndex(-splitIndex);
}

QSizeF Tiler::minimumSizeByIndex(int index) const
{
    Q_ASSERT(-static_cast<int>(splitMap_.size()) < index
             && index < static_cast<int>(tiles_.size()));
    if (index >= 0) {
        const auto &t = tiles_.at(static_cast<size_t>(index));
        if (!t.item)
            return { 0.0, 0.0 };
        const auto *a = tileAttached(t.item.get());
        if (!a)
            return { 0.0, 0.0 };
        return { a->minimumWidth(), a->minimumHeight() };
    } else {
        const auto &s = splitMap_.at(static_cast<size_t>(-index));
        return s.minimumSize;
    }
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
            a->setTiler(this);
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
        splitMap_.push_back(
                { orientation, { { tileIndex, 0.0 }, { tileIndex + 1, 0.5 } }, {}, {} });
        // split and b may be invalidated.
    }

    polish();
    emit countChanged();
}

void Tiler::moveTopLeftEdge(int tileIndex, Qt::Orientation orientation, qreal itemPos)
{
    if (tileIndex < 0 || tileIndex >= static_cast<int>(tiles_.size())) {
        qmlWarning(this) << "tile index out of range:" << tileIndex;
        return;
    }

    const auto [splitIndex, bandIndex] = findSplitBandByIndex(tileIndex, orientation);
    if (splitIndex < 0 || bandIndex <= 0) {
        qmlWarning(this) << "no movable edge found:" << tileIndex;
        return;
    }

    auto &split = splitMap_.at(static_cast<size_t>(splitIndex));
    const auto normalizedSize = [orientation, &split](const QSizeF &size) {
        return orientation == Qt::Horizontal ? size.width() / split.outerRect.width()
                                             : size.height() / split.outerRect.height();
    };
    const auto &prevBand = split.bands.at(static_cast<size_t>(bandIndex - 1));
    auto &targetBand = split.bands.at(static_cast<size_t>(bandIndex));

    const qreal minPos = prevBand.position + normalizedSize(minimumSizeByIndex(prevBand.index));
    const qreal nextPos = bandIndex + 1 < static_cast<int>(split.bands.size())
            ? split.bands.at(static_cast<size_t>(bandIndex + 1)).position
            : 1.0;
    const qreal maxPos = nextPos - normalizedSize(minimumSizeByIndex(targetBand.index));
    if (minPos > maxPos)
        return;

    const qreal exactPos = orientation == Qt::Horizontal
            ? (itemPos - split.outerRect.left()) / split.outerRect.width()
            : (itemPos - split.outerRect.top()) / split.outerRect.height();
    targetBand.position = std::clamp(exactPos, minPos, maxPos);
    polish();
}

void Tiler::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    polish();
}

void Tiler::updatePolish()
{
    accumulateTiles(0, 0);
    resizeTiles(0, itemRect(this), 0);
}

void Tiler::accumulateTiles(int splitIndex, int depth)
{
    Q_ASSERT_X(depth < static_cast<int>(splitMap_.size()), __FUNCTION__, "bad recursion detected");
    auto &split = splitMap_.at(static_cast<size_t>(splitIndex));
    qreal totalMinimumWidth = 0.0, totalMinimumHeight = 0.0;
    for (size_t i = 0; i < split.bands.size(); ++i) {
        const auto &band = split.bands.at(i);
        if (band.index < 0) {
            accumulateTiles(-band.index, depth + 1);
        }
        const auto min = minimumSizeByIndex(band.index);
        if (split.orientation == Qt::Horizontal) {
            totalMinimumWidth += min.width();
            totalMinimumHeight = std::max(min.height(), totalMinimumHeight);
        } else {
            totalMinimumWidth = std::max(min.width(), totalMinimumWidth);
            totalMinimumHeight += min.height();
        }
    }
    split.minimumSize = { totalMinimumWidth, totalMinimumHeight };
}

void Tiler::resizeTiles(int splitIndex, const QRectF &outerRect, int depth)
{
    Q_ASSERT_X(depth < static_cast<int>(splitMap_.size()), __FUNCTION__, "bad recursion detected");
    auto &split = splitMap_.at(static_cast<size_t>(splitIndex));
    split.outerRect = outerRect;

    const qreal boundStartPos =
            split.orientation == Qt::Horizontal ? outerRect.left() : outerRect.top();
    const qreal boundEndPos =
            split.orientation == Qt::Horizontal ? outerRect.right() : outerRect.bottom();

    // Calculate position and apply minimumWidth/Height from left/top to right/bottom.
    std::vector<qreal> itemPositions;
    itemPositions.reserve(split.bands.size() + 1);
    qreal boundPos = boundStartPos;
    for (const auto &band : split.bands) {
        const qreal exactPos = split.orientation == Qt::Horizontal
                ? outerRect.left() + band.position * outerRect.width()
                : outerRect.top() + band.position * outerRect.height();
        const qreal adjustedPos = std::max(exactPos, boundPos);
        itemPositions.push_back(adjustedPos);
        const auto min = minimumSizeByIndex(band.index);
        boundPos = adjustedPos + (split.orientation == Qt::Horizontal ? min.width() : min.height());
    }
    itemPositions.push_back(boundEndPos);

    // Apply minimumWidth/Height from right/bottom to left/top if position overflowed.
    boundPos = boundEndPos;
    for (size_t i = split.bands.size() - 1; i > 0; --i) {
        const auto &band = split.bands.at(i);
        const auto min = minimumSizeByIndex(band.index);
        boundPos -= split.orientation == Qt::Horizontal ? min.width() : min.height();
        if (itemPositions.at(i) <= boundPos)
            break;
        itemPositions.at(i) = boundPos;
    }

    for (size_t i = 0; i < split.bands.size(); ++i) {
        const auto &band = split.bands.at(i);
        const qreal s = itemPositions.at(i);
        const qreal e = itemPositions.at(i + 1);
        const auto rect = split.orientation == Qt::Horizontal
                ? QRectF(s, outerRect.y(), e - s, outerRect.height())
                : QRectF(outerRect.x(), s, outerRect.width(), e - s);
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

void TilerAttached::setTiler(Tiler *tiler)
{
    if (tiler_ == tiler)
        return;
    tiler_ = tiler;
    emit tilerChanged();
}

void TilerAttached::setIndex(int index)
{
    if (index_ == index)
        return;
    index_ = index;
    emit indexChanged();
}

void TilerAttached::setMinimumWidth(qreal width)
{
    if (qFuzzyCompare(minimumWidth_, width))
        return;
    minimumWidth_ = width;
    requestPolish();
    emit minimumWidthChanged();
}

void TilerAttached::setMinimumHeight(qreal height)
{
    if (qFuzzyCompare(minimumHeight_, height))
        return;
    minimumHeight_ = height;
    requestPolish();
    emit minimumHeightChanged();
}

void TilerAttached::requestPolish()
{
    if (!tiler_)
        return;
    tiler_->polish();
}
