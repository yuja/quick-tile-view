#include <QMouseEvent>
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
    std::vector<Band> bands;
    bands.push_back({ 0, 0.0, nullptr, nullptr });
    splitMap_.push_back({ Qt::Horizontal, std::move(bands), {}, {} });

    setAcceptedMouseButtons(Qt::LeftButton);
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
    for (size_t i = 0; i < tiles_.size(); ++i) {
        tiles_.at(i) = createTile(static_cast<int>(i));
    }
    polish();
}

auto Tiler::createTile(int index) -> Tile
{
    if (!tileDelegate_)
        return {};

    // See qquicksplitview.cpp
    auto *creationContext = tileDelegate_->creationContext();
    if (!creationContext)
        creationContext = qmlContext(this);
    auto context = std::make_unique<QQmlContext>(creationContext);
    context->setContextObject(this);

    auto *obj = tileDelegate_->beginCreate(context.get());
    if (auto item = std::unique_ptr<QQuickItem, ItemDeleter>(qobject_cast<QQuickItem *>(obj))) {
        item->setParentItem(this);
        if (auto *a = tileAttached(item.get())) {
            a->setTiler(this);
            a->setIndex(index);
        }
        tileDelegate_->completeCreate();
        return { std::move(item), std::move(context) };
    } else {
        qmlWarning(this) << "tile component does not create an item";
        delete obj;
        return {};
    }
}

void Tiler::setHorizontalHandle(QQmlComponent *handle)
{
    if (horizontalHandle_ == handle)
        return;
    horizontalHandle_ = handle;
    recreateHandles(Qt::Horizontal);
    emit horizontalHandleChanged();
}

void Tiler::setVerticalHandle(QQmlComponent *handle)
{
    if (verticalHandle_ == handle)
        return;
    verticalHandle_ = handle;
    recreateHandles(Qt::Vertical);
    emit verticalHandleChanged();
}

void Tiler::recreateHandles(Qt::Orientation orientation)
{
    if (orientation == Qt::Horizontal) {
        horizontalHandleWidth_ = 0.0;
    } else {
        verticalHandleHeight_ = 0.0;
    }
    for (auto &split : splitMap_) {
        if (split.orientation != orientation)
            continue;
        for (auto &band : split.bands) {
            band = createBand(band.index, band.position, orientation);
        }
    }
    polish();
}

auto Tiler::createBand(int index, qreal position, Qt::Orientation orientation) -> Band
{
    auto *component = (orientation == Qt::Horizontal ? horizontalHandle_ : verticalHandle_).get();
    if (!component || position <= 0.0)
        return { index, position, {}, {} };

    // See qquicksplitview.cpp
    auto *creationContext = component->creationContext();
    if (!creationContext)
        creationContext = qmlContext(this);
    auto context = std::make_unique<QQmlContext>(creationContext);
    context->setContextObject(this);

    auto *obj = component->beginCreate(context.get());
    if (auto item = std::unique_ptr<QQuickItem, ItemDeleter>(qobject_cast<QQuickItem *>(obj))) {
        item->setParentItem(this);
        component->completeCreate();
        // Apply identical width/height to all handles to make the layouter simple.
        if (orientation == Qt::Horizontal) {
            horizontalHandleWidth_ = item->implicitWidth();
        } else {
            verticalHandleHeight_ = item->implicitHeight();
        }
        return { index, position, std::move(item), std::move(context) };
    } else {
        qmlWarning(this) << "handle component does not create an item";
        delete obj;
        return { index, position, {}, {} };
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

std::tuple<int, int> Tiler::findSplitBandByHandleItem(const QQuickItem *item) const
{
    if (!item)
        return { -1, -1 };
    for (size_t i = 0; i < splitMap_.size(); ++i) {
        const auto &split = splitMap_.at(i);
        const auto p = std::find_if(split.bands.begin(), split.bands.end(),
                                    [item](const auto &b) { return b.handleItem.get() == item; });
        if (p == split.bands.end())
            continue;
        return { static_cast<int>(i), static_cast<int>(p - split.bands.begin()) };
    }
    return { -1, -1 };
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

    movingSplitIndex_ = -1;
    movingBandIndex_ = -1;
    movingSplitBandGrabOffset_ = {};

    // Insert new tile and adjust indices.
    tiles_.insert(tiles_.begin() + tileIndex + 1, createTile(tileIndex + 1));
    for (size_t i = static_cast<size_t>(tileIndex) + 2; i < tiles_.size(); ++i) {
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
        split.bands.insert(std::next(p),
                           createBand(tileIndex + 1, p->position + size / 2, orientation));
        // p and q may be invalidated.
    } else {
        std::vector<Band> subBands;
        subBands.push_back(createBand(tileIndex, 0.0, orientation));
        subBands.push_back(createBand(tileIndex + 1, 0.5, orientation));
        auto &b = split.bands.at(static_cast<size_t>(bandIndex));
        b.index = -static_cast<int>(splitMap_.size());
        splitMap_.push_back({ orientation, std::move(subBands), {}, {} });
        // split and b may be invalidated.
    }

    polish();
    emit countChanged();
}

void Tiler::close(int tileIndex)
{
    if (tileIndex < 0 || tileIndex >= static_cast<int>(tiles_.size())) {
        qmlWarning(this) << "tile index out of range:" << tileIndex;
        return;
    }
    if (tiles_.size() <= 1) {
        qmlWarning(this) << "cannot close the last tile";
        return;
    }

    movingSplitIndex_ = -1;
    movingBandIndex_ = -1;
    movingSplitBandGrabOffset_ = {};

    // Deallocate band of the tile to be removed.
    unlinkTileByIndex(splitMap_.at(0), tileIndex, 0);
    cleanTrailingEmptySplits();

    // Adjust tile indices and remove it.
    for (size_t i = static_cast<size_t>(tileIndex) + 1; i < tiles_.size(); ++i) {
        if (auto *a = tileAttached(tiles_.at(i).item.get())) {
            a->setIndex(static_cast<int>(i - 1));
        }
    }
    for (auto &split : splitMap_) {
        for (auto &b : split.bands) {
            if (b.index <= tileIndex)
                continue;
            b.index -= 1;
        }
    }
    tiles_.erase(tiles_.begin() + tileIndex);

    polish();
    emit countChanged();
}

bool Tiler::unlinkTileByIndex(Split &split, int index, int depth)
{
    Q_ASSERT_X(depth < static_cast<int>(splitMap_.size()), __FUNCTION__, "bad recursion detected");
    for (auto p = split.bands.begin(); p != split.bands.end(); ++p) {
        if (p->index == index) {
            Q_ASSERT(split.bands.size() >= 2);
            if (p == split.bands.begin()) {
                const auto q = std::next(p);
                q->position = 0.0;
                q->handleItem.reset();
                q->handleContext.reset();
            }
            split.bands.erase(p); // iterator gets invalidated.
            return true;
        }
        if (p->index >= 0)
            continue;
        auto &subSplit = splitMap_.at(static_cast<size_t>(-p->index));
        if (unlinkTileByIndex(subSplit, index, depth + 1)) {
            if (subSplit.bands.size() != 1)
                return true;
            p->index = subSplit.bands.back().index;
            subSplit.bands.pop_back(); // leave empty split so split refs wouldn't be invalidated.
            return true;
        }
    }
    return false;
}

/// Removes empty splits without remapping indices.
void Tiler::cleanTrailingEmptySplits()
{
    const auto p = std::find_if(splitMap_.rbegin(), splitMap_.rend(),
                                [](const auto &s) { return !s.bands.empty(); });
    splitMap_.erase(p.base(), splitMap_.end());
}

void Tiler::mousePressEvent(QMouseEvent *event)
{
    const auto *item = childAt(event->position().x(), event->position().y());
    std::tie(movingSplitIndex_, movingBandIndex_) = findSplitBandByHandleItem(item);
    if (movingSplitIndex_ < 0)
        return;
    movingSplitBandGrabOffset_ = event->position() - item->position();
    setKeepMouseGrab(true);
}

void Tiler::mouseMoveEvent(QMouseEvent *event)
{
    if (movingSplitIndex_ < 0)
        return;
    moveSplitBand(movingSplitIndex_, movingBandIndex_,
                  event->position() - movingSplitBandGrabOffset_);
}

void Tiler::mouseReleaseEvent(QMouseEvent * /*event*/)
{
    movingSplitIndex_ = -1;
    movingBandIndex_ = -1;
    movingSplitBandGrabOffset_ = {};
    setKeepMouseGrab(false);
}

void Tiler::moveSplitBand(int splitIndex, int bandIndex, const QPointF &itemPos)
{
    auto &split = splitMap_.at(static_cast<size_t>(splitIndex));

    // Extend bounds to have invisible 0th handle. See resizeTiles() for why.
    const bool isHorizontal = split.orientation == Qt::Horizontal;
    const qreal handleSize = isHorizontal ? horizontalHandleWidth_ : verticalHandleHeight_;
    const qreal boundStartPos =
            (isHorizontal ? split.outerRect.left() : split.outerRect.top()) - handleSize;
    const qreal boundEndPos = isHorizontal ? split.outerRect.right() : split.outerRect.bottom();
    const qreal boundSize = boundEndPos - boundStartPos;
    const auto normalizedBandSize = [this, isHorizontal, handleSize, boundSize](const Band &band) {
        const auto min = minimumSizeByIndex(band.index);
        return (handleSize + (isHorizontal ? min.width() : min.height())) / boundSize;
    };

    const auto &prevBand = split.bands.at(static_cast<size_t>(bandIndex - 1));
    auto &targetBand = split.bands.at(static_cast<size_t>(bandIndex));

    const qreal minPos = prevBand.position + normalizedBandSize(prevBand);
    const qreal nextPos = bandIndex + 1 < static_cast<int>(split.bands.size())
            ? split.bands.at(static_cast<size_t>(bandIndex + 1)).position
            : 1.0;
    const qreal maxPos = nextPos - normalizedBandSize(targetBand);
    if (minPos > maxPos)
        return;

    const qreal exactPos = ((isHorizontal ? itemPos.x() : itemPos.y()) - boundStartPos) / boundSize;
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
    qreal totalMinimumWidth = (split.orientation == Qt::Horizontal) ? -horizontalHandleWidth_ : 0.0;
    qreal totalMinimumHeight = (split.orientation != Qt::Horizontal) ? -verticalHandleHeight_ : 0.0;
    for (size_t i = 0; i < split.bands.size(); ++i) {
        const auto &band = split.bands.at(i);
        if (band.index < 0) {
            accumulateTiles(-band.index, depth + 1);
        }
        const auto min = minimumSizeByIndex(band.index);
        if (split.orientation == Qt::Horizontal) {
            totalMinimumWidth += horizontalHandleWidth_ + min.width();
            totalMinimumHeight = std::max(min.height(), totalMinimumHeight);
        } else {
            totalMinimumWidth = std::max(min.width(), totalMinimumWidth);
            totalMinimumHeight += verticalHandleHeight_ + min.height();
        }
    }
    split.minimumSize = { totalMinimumWidth, totalMinimumHeight };
}

void Tiler::resizeTiles(int splitIndex, const QRectF &outerRect, int depth)
{
    Q_ASSERT_X(depth < static_cast<int>(splitMap_.size()), __FUNCTION__, "bad recursion detected");
    auto &split = splitMap_.at(static_cast<size_t>(splitIndex));
    split.outerRect = outerRect;

    // Extend bounds to have invisible 0th handle.
    const bool isHorizontal = split.orientation == Qt::Horizontal;
    const qreal handleSize = isHorizontal ? horizontalHandleWidth_ : verticalHandleHeight_;
    const qreal boundStartPos = (isHorizontal ? outerRect.left() : outerRect.top()) - handleSize;
    const qreal boundEndPos = isHorizontal ? outerRect.right() : outerRect.bottom();
    const qreal boundSize = boundEndPos - boundStartPos;
    const auto minSize = [this, isHorizontal](const Band &band) {
        return isHorizontal ? minimumSizeByIndex(band.index).width()
                            : minimumSizeByIndex(band.index).height();
    };

    // Calculate position and apply minimumWidth/Height from left/top to right/bottom.
    // itemPositions[n] is the left/top corner of the handle belonging to the n-th tile.
    //
    // itemPositions[0]       itemPositions[1]       itemPositions[N]
    // +-------+--------------+-------+--------------+
    // :       |tile          |handle |tile          |
    // +-------:<--------->:          :<--------->:  :
    // :       :  minSize                minSize     :
    // :       :<------------------------------------:
    // :                      outerRect              :
    // :<------------------------------------------->:
    // boundStartPos      boundSize                  boundEndPos
    std::vector<qreal> itemPositions;
    itemPositions.reserve(split.bands.size() + 1);
    qreal boundPos = boundStartPos;
    for (const auto &band : split.bands) {
        const qreal exactPos = boundStartPos + band.position * boundSize;
        const qreal adjustedPos = std::max(exactPos, boundPos);
        itemPositions.push_back(adjustedPos);
        boundPos = adjustedPos + handleSize + minSize(band);
    }
    itemPositions.push_back(boundEndPos);

    // Apply minimumWidth/Height from right/bottom to left/top if position overflowed.
    boundPos = boundEndPos;
    for (size_t i = split.bands.size() - 1; i > 0; --i) {
        const auto &band = split.bands.at(i);
        boundPos -= handleSize + minSize(band);
        if (itemPositions.at(i) <= boundPos)
            break;
        itemPositions.at(i) = boundPos;
    }

    for (size_t i = 0; i < split.bands.size(); ++i) {
        const auto &band = split.bands.at(i);
        const qreal s = itemPositions.at(i);
        const qreal e = itemPositions.at(i + 1);
        const qreal m = handleSize;
        const auto handleRect = isHorizontal ? QRectF(s, outerRect.y(), m, outerRect.height())
                                             : QRectF(outerRect.x(), s, outerRect.width(), m);
        const auto contentRect = isHorizontal
                ? QRectF(s + m, outerRect.y(), e - (s + m), outerRect.height())
                : QRectF(outerRect.x(), s + m, outerRect.width(), e - (s + m));
        if (auto &item = band.handleItem) {
            item->setPosition(handleRect.topLeft());
            item->setSize(handleRect.size());
        }
        if (band.index >= 0) {
            if (auto &item = tiles_.at(static_cast<size_t>(band.index)).item) {
                item->setPosition(contentRect.topLeft());
                item->setSize(contentRect.size());
            }
        } else {
            resizeTiles(-band.index, contentRect, depth + 1);
        }
    }
}

void Tiler::ItemDeleter::operator()(QQuickItem *item) const
{
    if (!item)
        return;
    // Item must be alive while its signal handling is in progress.
    item->deleteLater();
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
