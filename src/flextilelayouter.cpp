#include <QMouseEvent>
#include <QtQml>
#include <algorithm>
#include <cmath>
#include <iterator>
#include "flextilelayouter.h"
#include "flextiler.h"

namespace {
FlexTilerAttached *tileAttached(QQuickItem *item)
{
    if (!item)
        return nullptr;
    return qobject_cast<FlexTilerAttached *>(qmlAttachedPropertiesObject<FlexTiler>(item));
}

constexpr qreal snapPixelSize = 5.0;

template<typename V>
qreal snapToVertices(const std::map<qreal, std::map<qreal, V>> &vertices, qreal key, qreal epsilon)
{
    const auto start = vertices.lower_bound(key - epsilon);
    if (start == vertices.end())
        return key;
    qreal bestKey = start->first;
    qreal bestDistance = std::abs(start->first - key);
    for (auto p = std::next(start); p != vertices.end(); ++p) {
        const qreal d = std::abs(p->first - key);
        if (d > bestDistance)
            break;
        bestKey = p->first;
        bestDistance = d;
    }
    return bestDistance <= epsilon ? bestKey : key;
}
}

FlexTileLayouter::FlexTileLayouter(QQuickItem *parent) : QQuickItem(parent)
{
    tiles_.push_back({ { 0.0, 0.0, 1.0, 1.0 }, {}, {}, {}, {}, {}, {} });
    setAcceptedMouseButtons(Qt::LeftButton);
}

FlexTileLayouter::~FlexTileLayouter() = default;

void FlexTileLayouter::setDelegate(QQmlComponent *delegate)
{
    if (tileDelegate_ == delegate)
        return;

    tileDelegate_ = delegate;
    recreateTiles();
    emit delegateChanged();
}

void FlexTileLayouter::setHorizontalHandle(QQmlComponent *handle)
{
    if (horizontalHandle_ == handle)
        return;
    horizontalHandle_ = handle;
    recreateTiles();
    emit horizontalHandleChanged();
}

void FlexTileLayouter::setVerticalHandle(QQmlComponent *handle)
{
    if (verticalHandle_ == handle)
        return;
    verticalHandle_ = handle;
    recreateTiles();
    emit verticalHandleChanged();
}

void FlexTileLayouter::recreateTiles()
{
    horizontalHandlePixelWidth_ = 0.0;
    verticalHandlePixelHeight_ = 0.0;
    for (size_t i = 0; i < tiles_.size(); ++i) {
        auto &tile = tiles_.at(i);
        tile = createTile(tile.normRect, static_cast<int>(i));
    }
    polish();
}

auto FlexTileLayouter::createTile(const KeyRect &normRect, int index) -> Tile
{
    auto [item, context] = createTileItem(index);
    auto [hHandleItem, hHandleContext] = createHandleItem(Qt::Horizontal);
    auto [vHandleItem, vHandleContext] = createHandleItem(Qt::Vertical);
    return {
        normRect,
        std::move(item),
        std::move(context),
        std::move(hHandleItem),
        std::move(hHandleContext),
        std::move(vHandleItem),
        std::move(vHandleContext),
    };
}

auto FlexTileLayouter::createTileItem(int index)
        -> std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>>
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
    if (auto item = UniqueItemPtr(qobject_cast<QQuickItem *>(obj))) {
        item->setParentItem(this);
        if (auto *a = tileAttached(item.get())) {
            // TODO: a->setTiler(this);
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

auto FlexTileLayouter::createHandleItem(Qt::Orientation orientation)
        -> std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>>
{
    auto *component = (orientation == Qt::Horizontal ? horizontalHandle_ : verticalHandle_).get();
    if (!component)
        return {};

    // See qquicksplitview.cpp
    auto *creationContext = component->creationContext();
    if (!creationContext)
        creationContext = qmlContext(this);
    auto context = std::make_unique<QQmlContext>(creationContext);
    context->setContextObject(this);

    auto *obj = component->beginCreate(context.get());
    if (auto item = UniqueItemPtr(qobject_cast<QQuickItem *>(obj))) {
        item->setParentItem(this);
        item->setVisible(false);
        component->completeCreate();
        // Apply identical width/height to all handles to make the layouter simple.
        if (orientation == Qt::Horizontal) {
            horizontalHandlePixelWidth_ = item->implicitWidth();
        } else {
            verticalHandlePixelHeight_ = item->implicitHeight();
        }
        return { std::move(item), std::move(context) };
    } else {
        qmlWarning(this) << "handle component does not create an item";
        delete obj;
        return {};
    }
}

std::tuple<int, Qt::Orientations>
FlexTileLayouter::findTileByHandleItem(const QQuickItem *item) const
{
    if (!item)
        return { -1, {} };
    for (size_t i = 0; i < tiles_.size(); ++i) {
        const auto &tile = tiles_.at(i);
        if (tile.horizontalHandleItem.get() == item)
            return { static_cast<int>(i), Qt::Horizontal };
        if (tile.verticalHandleItem.get() == item)
            return { static_cast<int>(i), Qt::Vertical };
        // If we had a corner handle, orientations would be Qt::Horizontal | Qt::Vertical.
    }
    return { -1, {} };
}

void FlexTileLayouter::split(size_t index, Qt::Orientation orientation,
                             std::vector<Tile> &&newTiles)
{
    // Invalidate grid of vertices, which will be recalculated later.
    xyVerticesMap_.clear();
    xyVerticesMap_.clear();
    resetMovingState();

    // Insert new tile and adjust indices. Unchanged (x, y) values must be preserved.
    // New borders may snap to existing vertices, but shouldn't move excessively compared
    // to the tile width/height. Otherwise the tiles would be stacked.
    const auto origRect = tiles_.at(index).normRect;
    if (orientation == Qt::Horizontal) {
        const qreal w = (origRect.x1 - origRect.x0) / static_cast<qreal>(newTiles.size() + 1);
        const qreal e = std::min(snapPixelSize / extendedOuterPixelRect().width(), 0.1 * w);
        std::vector<qreal> xs;
        for (size_t i = 0; i < newTiles.size(); ++i) {
            xs.push_back(
                    snapToVertices(xyVerticesMap_, origRect.x0 + static_cast<qreal>(i + 1) * w, e));
        }
        xs.push_back(origRect.x1);

        tiles_.at(index).normRect.x1 = xs.at(0);
        for (size_t i = 0; i < newTiles.size(); ++i) {
            const qreal x0 = xs.at(i);
            const qreal x1 = xs.at(i + 1);
            newTiles.at(i).normRect = { x0, origRect.y0, x1, origRect.y1 };
        }
    } else {
        const qreal h = (origRect.y1 - origRect.y0) / static_cast<qreal>(newTiles.size() + 1);
        const qreal e = std::min(snapPixelSize / extendedOuterPixelRect().height(), 0.1 * h);
        std::vector<qreal> ys;
        for (size_t i = 0; i < newTiles.size(); ++i) {
            ys.push_back(
                    snapToVertices(yxVerticesMap_, origRect.y0 + static_cast<qreal>(i + 1) * h, e));
        }
        ys.push_back(origRect.y1);

        tiles_.at(index).normRect.y1 = ys.at(0);
        for (size_t i = 0; i < newTiles.size(); ++i) {
            const qreal y0 = ys.at(i);
            const qreal y1 = ys.at(i + 1);
            newTiles.at(i).normRect = { origRect.x0, y0, origRect.x1, y1 };
        }
    }

    tiles_.insert(tiles_.begin() + static_cast<ptrdiff_t>(index) + 1,
                  std::make_move_iterator(newTiles.begin()),
                  std::make_move_iterator(newTiles.end()));
    for (size_t i = index + newTiles.size() + 1; i < tiles_.size(); ++i) {
        if (auto *a = tileAttached(tiles_.at(i).item.get())) {
            a->setIndex(static_cast<int>(i));
        }
    }

    polish();
    emit countChanged();
}

void FlexTileLayouter::close(int index)
{
    if (index < 0 || index >= static_cast<int>(tiles_.size())) {
        qmlWarning(this) << "tile index out of range:" << index;
        return;
    }

    const auto collectLine = [](auto linep, qreal pos0, qreal pos1) -> std::vector<int> {
        std::vector<int> indices;
        auto vp = linep->second.find(pos0);
        for (; vp != linep->second.end() && vp->first < pos1; ++vp) {
            Q_ASSERT(vp->second.tileIndex >= 0);
            indices.push_back(vp->second.tileIndex);
        }
        if (vp == linep->second.end() || vp->first > pos1)
            return {}; // unaligned tiles
        return indices;
    };

    const auto collectPrev = [collectLine](const VerticesMap &verticesMap, qreal key0, qreal pos0,
                                           qreal pos1) -> std::vector<int> {
        const auto linep = verticesMap.find(key0);
        if (linep == verticesMap.begin() || linep == verticesMap.end())
            return {};
        return collectLine(std::prev(linep), pos0, pos1);
    };

    const auto collectNext = [collectLine](const VerticesMap &verticesMap, qreal key1, qreal pos0,
                                           qreal pos1) -> std::vector<int> {
        const auto linep = verticesMap.find(key1);
        if (linep == verticesMap.end())
            return {};
        return collectLine(linep, pos0, pos1);
    };

    const auto origRect = tiles_.at(static_cast<size_t>(index)).normRect;
    if (const auto indices = collectPrev(xyVerticesMap_, origRect.x0, origRect.y0, origRect.y1);
        !indices.empty()) {
        // Found left matches, which will be expanded to right.
        for (const int i : indices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.x1 = origRect.x1;
        }
    } else if (const auto indices =
                       collectNext(xyVerticesMap_, origRect.x1, origRect.y0, origRect.y1);
               !indices.empty()) {
        // Found right matches, which will be expanded to left.
        for (const int i : indices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.x0 = origRect.x0;
        }
    } else if (const auto indices =
                       collectPrev(yxVerticesMap_, origRect.y0, origRect.x0, origRect.x1);
               !indices.empty()) {
        // Found top matches, which will be expanded to bottom.
        for (const int i : indices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.y1 = origRect.y1;
        }
    } else if (const auto indices =
                       collectNext(yxVerticesMap_, origRect.y1, origRect.x0, origRect.x1);
               !indices.empty()) {
        // Found bottom matches, which will be expanded to top.
        for (const int i : indices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.y0 = origRect.y0;
        }
    } else {
        qmlInfo(this) << "no collapsible tiles found for " << index;
        return;
    }

    // Invalidate grid of vertices, which will be recalculated later.
    xyVerticesMap_.clear();
    xyVerticesMap_.clear();
    resetMovingState();

    // Adjust tile indices and remove it.
    for (size_t i = static_cast<size_t>(index) + 1; i < tiles_.size(); ++i) {
        if (auto *a = tileAttached(tiles_.at(i).item.get())) {
            a->setIndex(static_cast<int>(i - 1));
        }
    }
    tiles_.erase(tiles_.begin() + index);

    polish();
    emit countChanged();
}

void FlexTileLayouter::mousePressEvent(QMouseEvent *event)
{
    const auto *item = childAt(event->position().x(), event->position().y());
    const auto [index, orientations] = findTileByHandleItem(item);
    if (index < 0)
        return;

    // Determine tiles to be moved on press because the grid may change while moving
    // the selected handle.
    movingTiles_ = collectAdjacentTiles(index, orientations);
    movableNormRect_ = calculateMovableNormRect(index, movingTiles_);
    movingHandleGrabPixelOffset_ = event->position() - item->position();
    preMoveXyVerticesMap_ = xyVerticesMap_;
    preMoveYxVerticesMap_ = yxVerticesMap_;
    setKeepMouseGrab(true);
}

void FlexTileLayouter::mouseMoveEvent(QMouseEvent *event)
{
    if (movingTiles_.left.empty() && movingTiles_.right.empty() && movingTiles_.top.empty()
        && movingTiles_.bottom.empty())
        return;
    if (movableNormRect_.isEmpty())
        return;

    const auto outerRect = extendedOuterPixelRect();
    const auto pixelPos = event->position() - movingHandleGrabPixelOffset_;
    const QPointF normPos((pixelPos.x() - outerRect.left()) / outerRect.width(),
                          (pixelPos.y() - outerRect.top()) / outerRect.height());
    const QPointF snappedNormPos(snapToVertices(preMoveXyVerticesMap_, normPos.x(),
                                                snapPixelSize / extendedOuterPixelRect().width()),
                                 snapToVertices(preMoveYxVerticesMap_, normPos.y(),
                                                snapPixelSize / extendedOuterPixelRect().height()));
    const QPointF clampedNormPos(
            std::clamp(snappedNormPos.x(), movableNormRect_.left(), movableNormRect_.right()),
            std::clamp(snappedNormPos.y(), movableNormRect_.top(), movableNormRect_.bottom()));
    moveAdjacentTiles(movingTiles_, clampedNormPos);
}

void FlexTileLayouter::mouseReleaseEvent(QMouseEvent * /*event*/)
{
    resetMovingState();
    setKeepMouseGrab(false);
}

void FlexTileLayouter::resetMovingState()
{
    movingTiles_ = {};
    movableNormRect_ = {};
    movingHandleGrabPixelOffset_ = {};
    preMoveXyVerticesMap_.clear();
    preMoveYxVerticesMap_.clear();
}

auto FlexTileLayouter::collectAdjacentTiles(int index, Qt::Orientations orientations) const
        -> AdjacentIndices
{
    const auto collect = [](const VerticesMap &vertices, qreal key1,
                            qreal pos0) -> std::tuple<std::vector<int>, std::vector<int>> {
        // Determine the right/bottom line from the handle item, and collect tiles
        // within the handle span.
        const auto line1 = vertices.find(key1);
        if (line1 == vertices.begin() || line1 == vertices.end())
            return {};
        const auto v1s = line1->second.find(pos0);
        if (v1s == line1->second.end())
            return {};
        const qreal pos1 = pos0 + v1s->second.normHandleSize;
        std::vector<int> tiles1;
        for (auto p = v1s; p != line1->second.end() && p->first < pos1; ++p) {
            Q_ASSERT(p->second.tileIndex >= 0);
            tiles1.push_back(p->second.tileIndex);
        }

        // Collect tiles on the adjacent left/top line within the same range.
        const auto line0 = std::prev(line1);
        const auto v0s = line0->second.find(pos0);
        if (v0s == line0->second.end())
            return {};
        std::vector<int> tiles0;
        for (auto p = v0s; p != line0->second.end() && p->first < pos1; ++p) {
            Q_ASSERT(p->second.tileIndex >= 0);
            tiles0.push_back(p->second.tileIndex);
        }

        return { tiles0, tiles1 };
    };

    AdjacentIndices indices;
    const auto &rect = tiles_.at(static_cast<size_t>(index)).normRect;
    if (orientations & Qt::Horizontal) {
        std::tie(indices.left, indices.right) = collect(xyVerticesMap_, rect.x0, rect.y0);
    }
    if (orientations & Qt::Vertical) {
        std::tie(indices.top, indices.bottom) = collect(yxVerticesMap_, rect.y0, rect.x0);
    }

    return indices;
}

QRectF FlexTileLayouter::calculateMovableNormRect(int index,
                                                  const AdjacentIndices &adjacentIndices) const
{
    const auto outerRect = extendedOuterPixelRect();
    const auto minimumTileWidth = [&outerRect](const Tile &tile) -> qreal {
        const auto *a = tileAttached(tile.item.get());
        return a ? a->minimumWidth() / outerRect.width() : 0.0;
    };
    const auto minimumTileHeight = [&outerRect](const Tile &tile) -> qreal {
        const auto *a = tileAttached(tile.item.get());
        return a ? a->minimumHeight() / outerRect.height() : 0.0;
    };

    qreal left = 0.0;
    qreal right = 1.0;
    qreal top = 0.0;
    qreal bottom = 1.0;

    for (const auto i : adjacentIndices.left) {
        const auto &tile = tiles_.at(static_cast<size_t>(i));
        const qreal x = tile.normRect.x0;
        left = std::max(x + minimumTileWidth(tile), left);
    }

    for (const auto i : adjacentIndices.right) {
        const auto &tile = tiles_.at(static_cast<size_t>(i));
        const qreal x = tile.normRect.x1;
        right = std::min(x, right);
    }

    for (const auto i : adjacentIndices.top) {
        const auto &tile = tiles_.at(static_cast<size_t>(i));
        const qreal y = tile.normRect.y0;
        top = std::max(y + minimumTileHeight(tile), top);
    }

    for (const auto i : adjacentIndices.bottom) {
        const auto &tile = tiles_.at(static_cast<size_t>(i));
        const qreal y = tile.normRect.y1;
        bottom = std::min(y, bottom);
    }

    const auto &targetTile = tiles_.at(static_cast<size_t>(index));
    const qreal marginX = horizontalHandlePixelWidth_ / outerRect.width();
    const qreal marginY = verticalHandlePixelHeight_ / outerRect.height();
    return {
        left + marginX,
        top + marginY,
        right - left - 2 * marginX - minimumTileWidth(targetTile),
        bottom - top - 2 * marginY - minimumTileHeight(targetTile),
    };
}

void FlexTileLayouter::moveAdjacentTiles(const AdjacentIndices &indices, const QPointF &normPos)
{
    for (const auto i : indices.left) {
        auto &tile = tiles_.at(static_cast<size_t>(i));
        tile.normRect.x1 = normPos.x();
    }

    for (const auto i : indices.right) {
        auto &tile = tiles_.at(static_cast<size_t>(i));
        tile.normRect.x0 = normPos.x();
    }

    for (const auto i : indices.top) {
        auto &tile = tiles_.at(static_cast<size_t>(i));
        tile.normRect.y1 = normPos.y();
    }

    for (const auto i : indices.bottom) {
        auto &tile = tiles_.at(static_cast<size_t>(i));
        tile.normRect.y0 = normPos.y();
    }

    polish();
}

void FlexTileLayouter::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    resetMovingState();
    polish();
}

void FlexTileLayouter::updatePolish()
{
    accumulateTiles(); // TODO: no need to rebuild vertices map on geometryChange()
    resizeTiles();
}

/// Outer bounds including invisible left-top handles.
QRectF FlexTileLayouter::extendedOuterPixelRect() const
{
    return {
        -horizontalHandlePixelWidth_,
        -verticalHandlePixelHeight_,
        width() + horizontalHandlePixelWidth_,
        height() + verticalHandlePixelHeight_,
    };
}

void FlexTileLayouter::accumulateTiles()
{
    // Collect all possible vertices.
    xyVerticesMap_.clear();
    yxVerticesMap_.clear();
    for (const auto &tile : tiles_) {
        const qreal x0 = tile.normRect.x0;
        const qreal y0 = tile.normRect.y0;
        xyVerticesMap_.insert({ x0, {} });
        yxVerticesMap_.insert({ y0, {} });
    }

    // Map tiles to vertices per axis
    //
    //       x0  xm  x1          horizontalVertices  verticalVertices
    //        '   '   '          x0  xm  x1
    //            |               '   '   '
    // y0- ---o---+---o---        A   +   B          y0- A-------B---
    //        |       |           |   :   |
    // ym- ---+   A   |           |   :   |          ym- +~~~~~~~+~~~
    //        |       | B         |   :   |
    // y1-    o-------+           C   +   |          y1- C-------+~~~
    //        |   C   |           |   :   |
    //
    // horizontalVertices {      // describes vertical lines made by horizontal splits
    //     x0: {y0, A}, {y1, C}  // A (x0, y0..y1), C (x0, y1..end)
    //     xm: {y0, -}, {y1, -}
    //     x1: {y0, B}           // B (x1, y0..end)
    // }
    // verticalVertices {        // describes horizontal lines made by vertical splits
    //     y0: {x0, A}, {x1, B}  // A (x0..x1, y0), B (x1..end, y0)
    //     ym: {x0, -}, {x1, -}
    //     y1: {x0, C}, {x1, -}  // C (x0..x1, y1)
    // }
    for (size_t i = 0; i < tiles_.size(); ++i) {
        auto &tile = tiles_.at(i);
        const qreal x0 = tile.normRect.x0;
        const qreal x1 = tile.normRect.x1;
        const qreal y0 = tile.normRect.y0;
        const qreal y1 = tile.normRect.y1;
        Q_ASSERT(x0 <= x1 && y0 <= y1);

        const auto h0 = xyVerticesMap_.lower_bound(x0);
        const auto h1 = xyVerticesMap_.lower_bound(x1);
        for (auto p = h0; p != h1; ++p) {
            p->second.insert({ y0, { static_cast<int>(i), 0.0, p == h0, false } });
        }

        const auto v0 = yxVerticesMap_.lower_bound(y0);
        const auto v1 = yxVerticesMap_.lower_bound(y1);
        for (auto p = v0; p != v1; ++p) {
            p->second.insert({ x0, { static_cast<int>(i), 0.0, p == v0, false } });
        }
    }

    // Insert terminators for convenience.
    for (auto &[x, vertices] : xyVerticesMap_) {
        vertices.insert({ 1.0, { -1, 0.0, false, false } });
    }
    for (auto &[y, vertices] : yxVerticesMap_) {
        vertices.insert({ 1.0, { -1, 0.0, false, false } });
    }

    // Calculate relation of adjacent tiles (e.g. handle span) per axis.
    const auto calculateAdjacentRelation = [](VerticesMap &vertices) {
        if (vertices.empty())
            return; // in case we allowed empty tiles.
        // First line should have no handle, so skipped updating handlePixelSize.
        auto line0 = vertices.begin();
        for (auto line1 = std::next(line0); line1 != vertices.end(); line0 = line1, ++line1) {
            auto v0s = line0->second.begin();
            auto v1s = line1->second.begin();
            Q_ASSERT(v0s != line0->second.end());
            Q_ASSERT(v1s != line1->second.end());
            auto v0p = std::next(v0s);
            auto v1p = std::next(v1s);
            int d0 = 1, d1 = 1;
            while (v0p != line0->second.end() && v1p != line1->second.end()) {
                // Handle can be isolated if two vertices of the adjacent lines meet.
                if (v0p->first == v1p->first) { // should exactly match here
                    v1s->second.normHandleSize = v1p->first - v1s->first;
                    // A single cell can be collapsed if the adjacent lines meet.
                    const bool border = v0s->second.tileIndex != v1s->second.tileIndex;
                    v0s->second.collapsible = v0s->second.collapsible || (d0 == 1 && border);
                    v1s->second.collapsible = v1s->second.collapsible || (d1 == 1 && border);
                    v0s = v0p;
                    v1s = v1p;
                    ++v0p;
                    ++v1p;
                    d0 = d1 = 1;
                } else if (v0p->first < v1p->first) {
                    ++v0p;
                    ++d0;
                } else {
                    ++v1p;
                    ++d1;
                }
            }
            Q_ASSERT(v0p == line0->second.end() && v1p == line1->second.end());
        }
    };
    calculateAdjacentRelation(xyVerticesMap_);
    calculateAdjacentRelation(yxVerticesMap_);
}

void FlexTileLayouter::resizeTiles()
{
    const auto outerRect = extendedOuterPixelRect();
    const auto mapToPixelX = [&outerRect](qreal x) {
        return outerRect.left() + x * outerRect.width();
    };
    const auto mapToPixelY = [&outerRect](qreal y) {
        return outerRect.top() + y * outerRect.height();
    };

    std::vector<bool> tilesCollapsible;
    tilesCollapsible.resize(tiles_.size(), false);

    // TODO: fix up pixel size per minimumWidth/Height

    for (const auto &[x, vertices] : xyVerticesMap_) {
        auto v0p = vertices.begin();
        if (v0p == vertices.end())
            continue;
        for (auto v1p = std::next(v0p); v1p != vertices.end(); v0p = v1p, ++v1p) {
            if (v0p->second.tileIndex < 0)
                continue;

            // Each tile may span more than one cells, and only the exterior cells can be
            // marked as collapsible.
            if (v0p->second.collapsible) {
                tilesCollapsible.at(static_cast<size_t>(v0p->second.tileIndex)) = true;
            }

            // No need to update items for all of the spanned cells.
            if (!v0p->second.primary)
                continue;
            const auto &tile = tiles_.at(static_cast<size_t>(v0p->second.tileIndex));
            if (auto &item = tile.item) {
                const qreal m = verticalHandlePixelHeight_;
                item->setY(mapToPixelY(v0p->first) + m);
                item->setHeight((v1p->first - v0p->first) * outerRect.height() - m);
            }
            if (auto &item = tile.horizontalHandleItem) {
                const qreal m = verticalHandlePixelHeight_;
                item->setVisible(v0p->second.normHandleSize > 0.0);
                item->setX(mapToPixelX(x));
                item->setY(mapToPixelY(v0p->first) + m);
                item->setWidth(horizontalHandlePixelWidth_);
                item->setHeight(v0p->second.normHandleSize * outerRect.height() - m);
            }
        }
    }

    for (const auto &[y, vertices] : yxVerticesMap_) {
        auto v0p = vertices.begin();
        if (v0p == vertices.end())
            continue;
        for (auto v1p = std::next(v0p); v1p != vertices.end(); v0p = v1p, ++v1p) {
            if (v0p->second.tileIndex < 0)
                continue;

            // Each tile may span more than one cells, and only the exterior cells can be
            // marked as collapsible.
            if (v0p->second.collapsible) {
                tilesCollapsible.at(static_cast<size_t>(v0p->second.tileIndex)) = true;
            }

            // No need to update items for all of the spanned cells.
            if (!v0p->second.primary)
                continue;
            const auto &tile = tiles_.at(static_cast<size_t>(v0p->second.tileIndex));
            if (auto &item = tile.item) {
                const qreal m = horizontalHandlePixelWidth_;
                item->setX(mapToPixelX(v0p->first) + m);
                item->setWidth((v1p->first - v0p->first) * outerRect.width() - m);
            }
            if (auto &item = tile.verticalHandleItem) {
                const qreal m = horizontalHandlePixelWidth_;
                item->setVisible(v0p->second.normHandleSize > 0.0);
                item->setX(mapToPixelX(v0p->first) + m);
                item->setY(mapToPixelY(y));
                item->setWidth(v0p->second.normHandleSize * outerRect.width() - m);
                item->setHeight(verticalHandlePixelHeight_);
            }
        }
    }

    for (size_t i = 0; i < tiles_.size(); ++i) {
        const auto &tile = tiles_.at(i);
        if (auto *a = tileAttached(tile.item.get())) {
            a->setClosable(tilesCollapsible.at(i));
        }
    }
}

void FlexTileLayouter::ItemDeleter::operator()(QQuickItem *item) const
{
    if (!item)
        return;
    // Item must be alive while its signal handling is in progress.
    item->deleteLater();
}
