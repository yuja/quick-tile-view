#include <QMouseEvent>
#include <QtQml>
#include <algorithm>
#include <cmath>
#include <iterator>
#include "flextiler.h"

namespace {
FlexTilerAttached *tileAttached(QQuickItem *item)
{
    if (!item)
        return nullptr;
    return qobject_cast<FlexTilerAttached *>(qmlAttachedPropertiesObject<FlexTiler>(item));
}
}

FlexTiler::FlexTiler(QQuickItem *parent) : QQuickItem(parent)
{
    tiles_.push_back({ { 0.0, 0.0, 1.0, 1.0 }, {}, {}, {}, {}, {}, {} });
    setAcceptedMouseButtons(Qt::LeftButton);
}

FlexTiler::~FlexTiler() = default;

FlexTilerAttached *FlexTiler::qmlAttachedProperties(QObject *object)
{
    return new FlexTilerAttached(object);
}

void FlexTiler::setDelegate(QQmlComponent *delegate)
{
    if (tileDelegate_ == delegate)
        return;

    tileDelegate_ = delegate;
    recreateTiles();
    emit delegateChanged();
}

void FlexTiler::setHorizontalHandle(QQmlComponent *handle)
{
    if (horizontalHandle_ == handle)
        return;
    horizontalHandle_ = handle;
    recreateTiles();
    emit horizontalHandleChanged();
}

void FlexTiler::setVerticalHandle(QQmlComponent *handle)
{
    if (verticalHandle_ == handle)
        return;
    verticalHandle_ = handle;
    recreateTiles();
    emit verticalHandleChanged();
}

void FlexTiler::recreateTiles()
{
    horizontalHandlePixelWidth_ = 0.0;
    verticalHandlePixelHeight_ = 0.0;
    for (size_t i = 0; i < tiles_.size(); ++i) {
        tiles_.at(i) = createTile(tiles_.at(i).normRect, static_cast<int>(i));
    }
    polish();
}

auto FlexTiler::createTile(const QRectF &normRect, int index) -> Tile
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

auto FlexTiler::createTileItem(int index) -> std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>>
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

auto FlexTiler::createHandleItem(Qt::Orientation orientation)
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

int FlexTiler::count() const
{
    return static_cast<int>(tiles_.size());
}

QQuickItem *FlexTiler::itemAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(tiles_.size())) {
        qmlWarning(this) << "tile index out of range:" << index;
        return nullptr;
    }

    return tiles_.at(static_cast<size_t>(index)).item.get();
}

std::tuple<int, Qt::Orientations> FlexTiler::findTileByHandleItem(const QQuickItem *item) const
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

void FlexTiler::split(int index, Qt::Orientation orientation, int count)
{
    if (index < 0 || index >= static_cast<int>(tiles_.size())) {
        qmlWarning(this) << "tile index out of range:" << index;
        return;
    }
    if (count < 2)
        return;

    // Invalidate grid of vertices, which will be recalculated later.
    horizontalVertices_.clear();
    horizontalVertices_.clear();
    resetMovingState();

    const auto srcRect = tiles_.at(static_cast<size_t>(index)).normRect;
    const qreal w = (orientation == Qt::Horizontal ? 1.0 / count : 1.0) * srcRect.width();
    const qreal h = (orientation == Qt::Horizontal ? 1.0 : 1.0 / count) * srcRect.height();
    const qreal xk = orientation == Qt::Horizontal ? 1.0 : 0.0;
    const qreal yk = orientation == Qt::Horizontal ? 0.0 : 1.0;

    // Insert new tile and adjust indices.
    tiles_.at(static_cast<size_t>(index)).normRect = { srcRect.x(), srcRect.y(), w, h };
    std::vector<Tile> newTiles;
    for (int i = 1; i < count; ++i) {
        // TODO: find closest (x, y) key from vertices map in pixel resolution
        const QRectF rect(srcRect.x() + i * xk * w, srcRect.y() + i * yk * h, w, h);
        newTiles.push_back(createTile(rect, index + i));
    }
    tiles_.insert(tiles_.begin() + index + 1, std::make_move_iterator(newTiles.begin()),
                  std::make_move_iterator(newTiles.end()));
    for (size_t i = static_cast<size_t>(index + count); i < tiles_.size(); ++i) {
        if (auto *a = tileAttached(tiles_.at(i).item.get())) {
            a->setIndex(static_cast<int>(i));
        }
    }

    polish();
    emit countChanged();
}

void FlexTiler::close(int index)
{
    if (index < 0 || index >= static_cast<int>(tiles_.size())) {
        qmlWarning(this) << "tile index out of range:" << index;
        return;
    }

    // TODO: extract utility function
    const auto findNextPos = [](const VerticesMap &verticesMap, qreal key, qreal pos) -> qreal {
        const auto line = verticesMap.find(key);
        if (line == verticesMap.end())
            return pos;
        const auto vp = line->second.upper_bound(pos);
        if (vp == line->second.end())
            return pos;
        return vp->first;
    };

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

    const QPointF orgPos = tiles_.at(static_cast<size_t>(index)).normRect.topLeft();
    const QPointF nextPos { findNextPos(verticalVertices_, orgPos.y(), orgPos.x()),
                            findNextPos(horizontalVertices_, orgPos.x(), orgPos.y()) };

    if (const auto indices = collectPrev(horizontalVertices_, orgPos.x(), orgPos.y(), nextPos.y());
        !indices.empty()) {
        // Found left matches, which will be expanded to right.
        for (const int i : indices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.setRight(nextPos.x());
        }
    } else if (const auto indices =
                       collectNext(horizontalVertices_, nextPos.x(), orgPos.y(), nextPos.y());
               !indices.empty()) {
        // Found right matches, which will be expanded to left.
        for (const int i : indices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.setLeft(orgPos.x());
        }
    } else if (const auto indices =
                       collectPrev(verticalVertices_, orgPos.y(), orgPos.x(), nextPos.x());
               !indices.empty()) {
        // Found top matches, which will be expanded to bottom.
        for (const int i : indices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.setBottom(nextPos.y());
        }
    } else if (const auto indices =
                       collectNext(verticalVertices_, nextPos.y(), orgPos.x(), nextPos.x());
               !indices.empty()) {
        // Found bottom matches, which will be expanded to top.
        for (const int i : indices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.setTop(orgPos.y());
        }
    } else {
        qmlInfo(this) << "no collapsible tiles found for " << index;
        return;
    }

    // Invalidate grid of vertices, which will be recalculated later.
    horizontalVertices_.clear();
    horizontalVertices_.clear();
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

void FlexTiler::mousePressEvent(QMouseEvent *event)
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
    setKeepMouseGrab(true);
}

void FlexTiler::mouseMoveEvent(QMouseEvent *event)
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
    const QPointF clampedNormPos(
            std::clamp(normPos.x(), movableNormRect_.left(), movableNormRect_.right()),
            std::clamp(normPos.y(), movableNormRect_.top(), movableNormRect_.bottom()));
    moveAdjacentTiles(movingTiles_, clampedNormPos);
}

void FlexTiler::mouseReleaseEvent(QMouseEvent * /*event*/)
{
    resetMovingState();
    setKeepMouseGrab(false);
}

void FlexTiler::resetMovingState()
{
    movingTiles_ = {};
    movableNormRect_ = {};
    movingHandleGrabPixelOffset_ = {};
}

auto FlexTiler::collectAdjacentTiles(int index, Qt::Orientations orientations) const
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
    const auto pos = tiles_.at(static_cast<size_t>(index)).normRect.topLeft();
    if (orientations & Qt::Horizontal) {
        std::tie(indices.left, indices.right) = collect(horizontalVertices_, pos.x(), pos.y());
    }
    if (orientations & Qt::Vertical) {
        std::tie(indices.top, indices.bottom) = collect(verticalVertices_, pos.y(), pos.x());
    }

    return indices;
}

QRectF FlexTiler::calculateMovableNormRect(int index, const AdjacentIndices &adjacentIndices) const
{
    // TODO: extract utility function
    const auto findNextPos = [](const VerticesMap &vertices, qreal key, qreal pos) -> qreal {
        const auto line = vertices.find(key);
        if (line == vertices.end())
            return pos;
        const auto v1p = line->second.upper_bound(pos);
        if (v1p == line->second.end())
            return pos;
        return v1p->first;
    };

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
        const qreal x = tile.normRect.x();
        left = std::max(x + minimumTileWidth(tile), left);
    }

    for (const auto i : adjacentIndices.right) {
        const auto &tile = tiles_.at(static_cast<size_t>(i));
        const qreal x = findNextPos(verticalVertices_, tile.normRect.y(), tile.normRect.x());
        right = std::min(x, right);
    }

    for (const auto i : adjacentIndices.top) {
        const auto &tile = tiles_.at(static_cast<size_t>(i));
        const qreal y = tile.normRect.y();
        top = std::max(y + minimumTileHeight(tile), top);
    }

    for (const auto i : adjacentIndices.bottom) {
        const auto &tile = tiles_.at(static_cast<size_t>(i));
        const qreal y = findNextPos(horizontalVertices_, tile.normRect.x(), tile.normRect.y());
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

void FlexTiler::moveAdjacentTiles(const AdjacentIndices &indices, const QPointF &normPos)
{
    // TODO: snap to (x, y) key of vertices map

    for (const auto i : indices.left) {
        auto &tile = tiles_.at(static_cast<size_t>(i));
        tile.normRect.setRight(normPos.x());
    }

    for (const auto i : indices.right) {
        auto &tile = tiles_.at(static_cast<size_t>(i));
        tile.normRect.setLeft(normPos.x());
    }

    for (const auto i : indices.top) {
        auto &tile = tiles_.at(static_cast<size_t>(i));
        tile.normRect.setBottom(normPos.y());
    }

    for (const auto i : indices.bottom) {
        auto &tile = tiles_.at(static_cast<size_t>(i));
        tile.normRect.setTop(normPos.y());
    }

    polish();
}

void FlexTiler::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    resetMovingState();
    polish();
}

void FlexTiler::updatePolish()
{
    accumulateTiles(); // TODO: no need to rebuild vertices map on geometryChange()
    resizeTiles();
}

/// Outer bounds including invisible left-top handles.
QRectF FlexTiler::extendedOuterPixelRect() const
{
    return {
        -horizontalHandlePixelWidth_,
        -verticalHandlePixelHeight_,
        width() + horizontalHandlePixelWidth_,
        height() + verticalHandlePixelHeight_,
    };
}

void FlexTiler::accumulateTiles()
{
    // Collect all possible vertices.
    horizontalVertices_.clear();
    verticalVertices_.clear();
    for (const auto &tile : tiles_) {
        const qreal x0 = tile.normRect.left();
        const qreal y0 = tile.normRect.top();
        horizontalVertices_.insert({ x0, {} });
        verticalVertices_.insert({ y0, {} });
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
        const qreal x0 = tile.normRect.left();
        const qreal x1 = tile.normRect.right();
        const qreal y0 = tile.normRect.top();
        const qreal y1 = tile.normRect.bottom();
        Q_ASSERT(x0 <= x1 && y0 <= y1);

        const auto h0 = horizontalVertices_.lower_bound(x0);
        const auto h1 = horizontalVertices_.lower_bound(x1);
        for (auto p = h0; p != h1; ++p) {
            p->second.insert({ y0, { static_cast<int>(i), 0.0, p == h0, false } });
        }

        const auto v0 = verticalVertices_.lower_bound(y0);
        const auto v1 = verticalVertices_.lower_bound(y1);
        for (auto p = v0; p != v1; ++p) {
            p->second.insert({ x0, { static_cast<int>(i), 0.0, p == v0, false } });
        }
    }

    // Insert terminators for convenience.
    for (auto &[x, vertices] : horizontalVertices_) {
        vertices.insert({ 1.0, { -1, 0.0, false, false } });
    }
    for (auto &[y, vertices] : verticalVertices_) {
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
    calculateAdjacentRelation(horizontalVertices_);
    calculateAdjacentRelation(verticalVertices_);
}

void FlexTiler::resizeTiles()
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

    for (const auto &[x, vertices] : horizontalVertices_) {
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

    for (const auto &[y, vertices] : verticalVertices_) {
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

void FlexTiler::ItemDeleter::operator()(QQuickItem *item) const
{
    if (!item)
        return;
    // Item must be alive while its signal handling is in progress.
    item->deleteLater();
}

FlexTilerAttached::FlexTilerAttached(QObject *parent) : QObject(parent) { }

void FlexTilerAttached::setTiler(FlexTiler *tiler)
{
    if (tiler_ == tiler)
        return;
    tiler_ = tiler;
    emit tilerChanged();
}

void FlexTilerAttached::setIndex(int index)
{
    if (index_ == index)
        return;
    index_ = index;
    emit indexChanged();
}

void FlexTilerAttached::setMinimumWidth(qreal width)
{
    if (qFuzzyCompare(minimumWidth_, width))
        return;
    minimumWidth_ = width;
    requestPolish();
    emit minimumWidthChanged();
}

void FlexTilerAttached::setMinimumHeight(qreal height)
{
    if (qFuzzyCompare(minimumHeight_, height))
        return;
    minimumHeight_ = height;
    requestPolish();
    emit minimumHeightChanged();
}

void FlexTilerAttached::requestPolish()
{
    if (!tiler_)
        return;
    tiler_->polish();
}

void FlexTilerAttached::setClosable(bool closable)
{
    if (closable_ == closable)
        return;
    closable_ = closable;
    emit closableChanged();
}
