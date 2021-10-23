#include <QMouseEvent>
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <numeric>
#include "flextilelayouter.h"
#include "flextiler.h"

namespace {
FlexTilerAttached *tileAttached(const QQuickItem *item)
{
    if (!item)
        return nullptr;
    return qobject_cast<FlexTilerAttached *>(qmlAttachedPropertiesObject<FlexTiler>(item));
}

qreal snapToVertices(const FlexTileLayouter::VerticesMap &verticesMap, qreal key, qreal epsilon)
{
    const auto start = verticesMap.lower_bound(key - epsilon);
    if (start == verticesMap.end())
        return key;
    qreal bestKey = start->first;
    qreal bestDistance = std::abs(start->first - key);
    for (auto p = std::next(start); p != verticesMap.end(); ++p) {
        const qreal d = std::abs(p->first - key);
        if (d > bestDistance)
            break;
        bestKey = p->first;
        bestDistance = d;
    }
    return bestDistance <= epsilon ? bestKey : key;
}
}

FlexTileLayouter::FlexTileLayouter()
{
    tiles_.push_back({ { 0.0, 0.0, 1.0, 1.0 }, {}, {}, {}, {}, {}, {} });
}

FlexTileLayouter::~FlexTileLayouter() = default;

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
                             std::vector<Tile> &&newTiles, const QSizeF &snapSize)
{
    resetMovingState();
    ensureVerticesMapBuilt();

    // Insert new tile and adjust indices. Unchanged (x, y) values must be preserved.
    // New borders may snap to existing vertices, but shouldn't move excessively compared
    // to the tile width/height. Otherwise the tiles would be stacked.
    const auto origRect = tiles_.at(index).normRect;
    if (orientation == Qt::Horizontal) {
        const qreal w = (origRect.x1 - origRect.x0) / static_cast<qreal>(newTiles.size() + 1);
        const qreal e = std::min(snapSize.width(), 0.1 * w);
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
        const qreal e = std::min(snapSize.height(), 0.1 * h);
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

    invalidateVerticesMap();
}

/*!
 * Closes the specified tile and collapses the adjacent tiles to fill the area.
 *
 * Returns index of one of the tiles filled the closed area, or -1 if the tile
 * couldn't be collapsed to any of the adjacent tiles.
 */
int FlexTileLayouter::close(size_t index)
{
    resetMovingState();
    ensureVerticesMapBuilt();

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

    const auto avgIndexDistance = [index](const std::vector<int> &toIndices) {
        if (toIndices.empty())
            return std::numeric_limits<qreal>::max();
        const int d = std::accumulate(toIndices.begin(), toIndices.end(), 0, [index](int n, int i) {
            return n + std::abs(i - static_cast<int>(index));
        });
        return static_cast<qreal>(d) / static_cast<qreal>(toIndices.size());
    };

    const auto origRect = tiles_.at(static_cast<size_t>(index)).normRect;
    const std::array<std::vector<int>, 4> collectedIndices {
        collectPrev(xyVerticesMap_, origRect.x0, origRect.y0, origRect.y1), // left
        collectNext(xyVerticesMap_, origRect.x1, origRect.y0, origRect.y1), // right
        collectPrev(yxVerticesMap_, origRect.y0, origRect.x0, origRect.x1), // top
        collectNext(yxVerticesMap_, origRect.y1, origRect.x0, origRect.x1), // bottom
    };
    // If there are multiple candidates, pick the closest in the tiles vector so
    // the tiles previously split will likely be merged.
    auto bestIndices = collectedIndices.end();
    qreal bestIndexDistance = std::numeric_limits<qreal>::max();
    for (auto p = collectedIndices.begin(); p != collectedIndices.end(); ++p) {
        const qreal d = avgIndexDistance(*p);
        if (d >= bestIndexDistance)
            continue;
        bestIndices = p;
        bestIndexDistance = d;
    }
    if (bestIndices == collectedIndices.end())
        return -1;
    Q_ASSERT(!bestIndices->empty());
    switch (bestIndices - collectedIndices.begin()) {
    case 0:
        // Found left matches, which will be expanded to right.
        for (const int i : *bestIndices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.x1 = origRect.x1;
        }
        break;
    case 1:
        // Found right matches, which will be expanded to left.
        for (const int i : *bestIndices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.x0 = origRect.x0;
        }
        break;
    case 2:
        // Found top matches, which will be expanded to bottom.
        for (const int i : *bestIndices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.y1 = origRect.y1;
        }
        break;
    case 3:
        // Found bottom matches, which will be expanded to top.
        for (const int i : *bestIndices) {
            auto &tile = tiles_.at(static_cast<size_t>(i));
            tile.normRect.y0 = origRect.y0;
        }
        break;
    }

    tiles_.erase(tiles_.begin() + static_cast<ptrdiff_t>(index));

    invalidateVerticesMap();
    return bestIndices->front() - static_cast<int>(bestIndices->front() >= static_cast<int>(index));
}

bool FlexTileLayouter::isMoving() const
{
    return !movingTiles_.left.empty() || !movingTiles_.right.empty() || !movingTiles_.top.empty()
            || !movingTiles_.bottom.empty();
}

void FlexTileLayouter::startMoving(size_t index, Qt::Orientations orientations, bool lineThrough,
                                   const QRectF &outerPixelRect, const QSizeF &handlePixelSize)
{
    ensureVerticesMapBuilt();
    movingTiles_ = lineThrough ? collectAdjacentTilesThrough(index, orientations)
                               : collectAdjacentTiles(index, orientations);
    movableNormRect_ =
            calculateMovableNormRect(index, movingTiles_, outerPixelRect, handlePixelSize);
    preMoveXyVerticesMap_ = xyVerticesMap_;
    preMoveYxVerticesMap_ = yxVerticesMap_;
}

void FlexTileLayouter::moveTo(const QPointF &normPos, const QSizeF &snapSize)
{
    Q_ASSERT(isMoving());
    if (movableNormRect_.isEmpty())
        return;
    const QPointF snappedNormPos(
            snapToVertices(preMoveXyVerticesMap_, normPos.x(), snapSize.width()),
            snapToVertices(preMoveYxVerticesMap_, normPos.y(), snapSize.height()));
    const QPointF clampedNormPos(
            std::clamp(snappedNormPos.x(), movableNormRect_.left(), movableNormRect_.right()),
            std::clamp(snappedNormPos.y(), movableNormRect_.top(), movableNormRect_.bottom()));
    moveAdjacentTiles(movingTiles_, clampedNormPos);
}

void FlexTileLayouter::resetMovingState()
{
    movingTiles_ = {};
    movableNormRect_ = {};
    preMoveXyVerticesMap_.clear();
    preMoveYxVerticesMap_.clear();
}

auto FlexTileLayouter::collectAdjacentTiles(size_t index, Qt::Orientations orientations) const
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
        const qreal pos1 = v1s->second.handleEnd;
        std::vector<int> tiles1;
        for (auto p = v1s; p != line1->second.end() && p->first < pos1; ++p) {
            Q_ASSERT(p->second.tileIndex >= 0);
            tiles1.push_back(p->second.tileIndex);
        }

        // Collect tiles on the adjacent left/top line within the same range.
        const auto line0 = std::prev(line1);
        std::vector<int> tiles0;
        for (auto p = line0->second.find(pos0); p != line0->second.end() && p->first < pos1; ++p) {
            Q_ASSERT(p->second.tileIndex >= 0);
            tiles0.push_back(p->second.tileIndex);
        }

        return { tiles0, tiles1 };
    };

    AdjacentIndices indices;
    const auto &rect = tiles_.at(index).normRect;
    if (orientations & Qt::Horizontal) {
        std::tie(indices.left, indices.right) = collect(xyVerticesMap_, rect.x0, rect.y0);
    }
    if (orientations & Qt::Vertical) {
        std::tie(indices.top, indices.bottom) = collect(yxVerticesMap_, rect.y0, rect.x0);
    }

    return indices;
}

auto FlexTileLayouter::collectAdjacentTilesThrough(size_t index,
                                                   Qt::Orientations orientations) const
        -> AdjacentIndices
{
    const auto collect = [](const VerticesMap &vertices, qreal key1,
                            qreal pos) -> std::tuple<std::vector<int>, std::vector<int>> {
        // Walk through the line to determine contiguous range including the source item.
        const auto line1 = vertices.find(key1);
        if (line1 == vertices.begin() || line1 == vertices.end())
            return {};
        qreal pos0 = 1.0, pos1 = 0.0;
        std::vector<int> tiles1;
        for (auto p = line1->second.begin(); p != line1->second.end(); ++p) {
            if (!p->second.primary && p->first > pos) { // reached to right/bottom edge
                pos1 = p->first;
                break;
            } else if (!p->second.primary) { // not contiguous to the source item
                pos0 = 1.0;
                tiles1.clear();
                continue;
            }
            Q_ASSERT(p->second.tileIndex >= 0);
            if (tiles1.empty()) {
                pos0 = p->first;
            }
            tiles1.push_back(p->second.tileIndex);
        }

        // Collect tiles on the adjacent left/top line within the same range.
        const auto line0 = std::prev(line1);
        std::vector<int> tiles0;
        for (auto p = line0->second.find(pos0); p != line0->second.end() && p->first < pos1; ++p) {
            Q_ASSERT(p->second.tileIndex >= 0);
            tiles0.push_back(p->second.tileIndex);
        }

        return { tiles0, tiles1 };
    };

    AdjacentIndices indices;
    const auto &rect = tiles_.at(index).normRect;
    if (orientations & Qt::Horizontal) {
        std::tie(indices.left, indices.right) = collect(xyVerticesMap_, rect.x0, rect.y0);
    }
    if (orientations & Qt::Vertical) {
        std::tie(indices.top, indices.bottom) = collect(yxVerticesMap_, rect.y0, rect.x0);
    }

    return indices;
}

QRectF FlexTileLayouter::calculateMovableNormRect(size_t index,
                                                  const AdjacentIndices &adjacentIndices,
                                                  const QRectF &outerPixelRect,
                                                  const QSizeF &handlePixelSize) const
{
    const auto minimumTileWidth = [&outerPixelRect](const Tile &tile) -> qreal {
        const auto *a = tileAttached(tile.item.get());
        return a ? a->minimumWidth() / outerPixelRect.width() : 0.0;
    };
    const auto minimumTileHeight = [&outerPixelRect](const Tile &tile) -> qreal {
        const auto *a = tileAttached(tile.item.get());
        return a ? a->minimumHeight() / outerPixelRect.height() : 0.0;
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

    const auto &targetTile = tiles_.at(index);
    const qreal marginX = handlePixelSize.width() / outerPixelRect.width();
    const qreal marginY = handlePixelSize.height() / outerPixelRect.height();
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

    invalidateVerticesMap();
}

void FlexTileLayouter::invalidateVerticesMap()
{
    xyVerticesMap_.clear();
    yxVerticesMap_.clear();
    tilesCollapsible_.clear();
}

void FlexTileLayouter::ensureVerticesMapBuilt()
{
    Q_ASSERT(!tiles_.empty());
    if (!xyVerticesMap_.empty())
        return;

    // Collect all possible vertices.
    Q_ASSERT(xyVerticesMap_.empty() && yxVerticesMap_.empty());
    for (const auto &tile : tiles_) {
        const qreal x0 = tile.normRect.x0;
        const qreal y0 = tile.normRect.y0;
        xyVerticesMap_.insert({ x0, {} });
        yxVerticesMap_.insert({ y0, {} });
    }

    // Map tiles to vertices per axis
    //
    //       x0  xm  x1          xyVertices          yxVertices
    //        '   '   '          x0  xm  x1
    //            |               '   '   '
    // y0- ---o---+---o---        A   +   B          y0- A-------B---
    //        |       |           |   :   |
    // ym- ---+   A   |           |   :   |          ym- +~~~~~~~+~~~
    //        |       | B         |   :   |
    // y1-    o-------+           C   +   |          y1- C-------+~~~
    //        |   C   |           |   :   |
    //
    // xyVertices {              // describes vertical lines made by horizontal splits
    //     x0: {y0, A}, {y1, C}  // A (x0, y0..y1), C (x0, y1..end)
    //     xm: {y0, -}, {y1, -}
    //     x1: {y0, B}           // B (x1, y0..end)
    // }
    // yxVertices {              // describes horizontal lines made by vertical splits
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
            p->second.insert({ y0, { static_cast<int>(i), p == h0, y0 } });
        }

        const auto v0 = yxVerticesMap_.lower_bound(y0);
        const auto v1 = yxVerticesMap_.lower_bound(y1);
        for (auto p = v0; p != v1; ++p) {
            p->second.insert({ x0, { static_cast<int>(i), p == v0, x0 } });
        }
    }

    // Insert terminators for convenience.
    for (auto &[x, vertices] : xyVerticesMap_) {
        vertices.insert({ 1.0, { -1, false, 1.0 } });
    }
    for (auto &[y, vertices] : yxVerticesMap_) {
        vertices.insert({ 1.0, { -1, false, 1.0 } });
    }

    // Calculate relation of adjacent tiles (e.g. handle span) per axis.
    Q_ASSERT(tilesCollapsible_.empty());
    tilesCollapsible_.resize(tiles_.size(), false);
    const auto calculateAdjacentRelation = [this](VerticesMap &vertices) {
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
                    Q_ASSERT(v0s->second.tileIndex >= 0 && v1s->second.tileIndex >= 0);
                    v1s->second.handleEnd = v1p->first;
                    // A single cell can be collapsed if the adjacent lines meet.
                    const bool border = v0s->second.tileIndex != v1s->second.tileIndex;
                    if (d0 == 1 && border) {
                        tilesCollapsible_.at(static_cast<size_t>(v0s->second.tileIndex)) = true;
                    }
                    if (d1 == 1 && border) {
                        tilesCollapsible_.at(static_cast<size_t>(v1s->second.tileIndex)) = true;
                    }
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

void FlexTileLayouter::resizeTiles(const QRectF &outerPixelRect, const QSizeF &handlePixelSize)
{
    ensureVerticesMapBuilt();

    // Avoid sub-pixel alignment of tiles. Be aware that outerPixelRect may start
    // from a negative point and std::round() would round it away from zero, which
    // is not what we want.
    const auto mapToPixelX = [&outerPixelRect](qreal x) {
        return outerPixelRect.left() + std::round(x * outerPixelRect.width());
    };
    const auto mapToPixelY = [&outerPixelRect](qreal y) {
        return outerPixelRect.top() + std::round(y * outerPixelRect.height());
    };

    // TODO: fix up pixel size per minimumWidth/Height

    for (const auto &[x, vertices] : xyVerticesMap_) {
        auto v0p = vertices.begin();
        if (v0p == vertices.end())
            continue;
        for (auto v1p = std::next(v0p); v1p != vertices.end(); v0p = v1p, ++v1p) {
            Q_ASSERT(v0p->second.tileIndex >= 0);
            // No need to update items for all of the spanned cells.
            if (!v0p->second.primary)
                continue;
            const auto &tile = tiles_.at(static_cast<size_t>(v0p->second.tileIndex));
            if (auto &item = tile.item) {
                const qreal m = handlePixelSize.height();
                item->setY(mapToPixelY(v0p->first) + m);
                item->setHeight(mapToPixelY(v1p->first) - mapToPixelY(v0p->first) - m);
            }
            if (auto &item = tile.horizontalHandleItem) {
                const qreal m = handlePixelSize.height();
                item->setVisible(v0p->second.handleEnd > v0p->first);
                item->setX(mapToPixelX(x));
                item->setY(mapToPixelY(v0p->first) + m);
                item->setWidth(handlePixelSize.width());
                item->setHeight(mapToPixelY(v0p->second.handleEnd) - mapToPixelY(v0p->first) - m);
            }
        }
    }

    for (const auto &[y, vertices] : yxVerticesMap_) {
        auto v0p = vertices.begin();
        if (v0p == vertices.end())
            continue;
        for (auto v1p = std::next(v0p); v1p != vertices.end(); v0p = v1p, ++v1p) {
            Q_ASSERT(v0p->second.tileIndex >= 0);
            // No need to update items for all of the spanned cells.
            if (!v0p->second.primary)
                continue;
            const auto &tile = tiles_.at(static_cast<size_t>(v0p->second.tileIndex));
            if (auto &item = tile.item) {
                const qreal m = handlePixelSize.width();
                item->setX(mapToPixelX(v0p->first) + m);
                item->setWidth(mapToPixelX(v1p->first) - mapToPixelX(v0p->first) - m);
            }
            if (auto &item = tile.verticalHandleItem) {
                const qreal m = handlePixelSize.width();
                item->setVisible(v0p->second.handleEnd > v0p->first);
                item->setX(mapToPixelX(v0p->first) + m);
                item->setY(mapToPixelY(y));
                item->setWidth(mapToPixelX(v0p->second.handleEnd) - mapToPixelX(v0p->first) - m);
                item->setHeight(handlePixelSize.height());
            }
        }
    }

    for (size_t i = 0; i < tiles_.size(); ++i) {
        const auto &tile = tiles_.at(i);
        if (auto *a = tileAttached(tile.item.get())) {
            a->setClosable(tilesCollapsible_.at(i));
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
