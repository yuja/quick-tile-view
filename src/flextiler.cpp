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
    horizontalHandleWidth_ = 0.0;
    verticalHandleHeight_ = 0.0;
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
            horizontalHandleWidth_ = item->implicitWidth();
        } else {
            verticalHandleHeight_ = item->implicitHeight();
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

    const auto srcRect = tiles_.at(static_cast<size_t>(index)).normRect;
    const qreal w = (orientation == Qt::Horizontal ? 1.0 / count : 1.0) * srcRect.width();
    const qreal h = (orientation == Qt::Horizontal ? 1.0 : 1.0 / count) * srcRect.height();
    const qreal xk = orientation == Qt::Horizontal ? 1.0 : 0.0;
    const qreal yk = orientation == Qt::Horizontal ? 0.0 : 1.0;

    // Insert new tile and adjust indices.
    tiles_.at(static_cast<size_t>(index)).normRect = { srcRect.x(), srcRect.y(), w, h };
    std::vector<Tile> newTiles;
    for (int i = 1; i < count; ++i) {
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

void FlexTiler::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    polish();
}

void FlexTiler::updatePolish()
{
    accumulateTiles();
    resizeTiles();
}

/// Outer bounds including invisible left-top handles.
QRectF FlexTiler::extendedOuterRect() const
{
    return {
        -horizontalHandleWidth_,
        -verticalHandleHeight_,
        width() + horizontalHandleWidth_,
        height() + verticalHandleHeight_,
    };
}

void FlexTiler::accumulateTiles()
{
    const auto outerRect = extendedOuterRect();
    // Align to pixel border so separate borders can be connected.
    const auto mapToPixelX = [&outerRect](qreal x) {
        return static_cast<int>(std::lround(outerRect.left() + x * outerRect.width()));
    };
    const auto mapToPixelY = [&outerRect](qreal y) {
        return static_cast<int>(std::lround(outerRect.top() + y * outerRect.height()));
    };

    // Collect all possible vertices.
    horizontalVertices_.clear();
    verticalVertices_.clear();
    for (const auto &tile : tiles_) {
        const int x0 = mapToPixelX(tile.normRect.left());
        const int y0 = mapToPixelY(tile.normRect.top());
        horizontalVertices_.insert({ x0, {} });
        verticalVertices_.insert({ y0, {} });
    }

    // Map tiles to vertices per axis
    //
    //       x0  xm  x1
    //        '   '   '
    //            |
    // y0- ---o---+---o---
    //        |       |
    // ym- ---+   A   |
    //        |       | B
    // y1-    o-------+
    //        |   C   |
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
        const auto &tile = tiles_.at(i);
        const int x0 = mapToPixelX(tile.normRect.left());
        const int x1 = mapToPixelX(tile.normRect.right());
        const int y0 = mapToPixelY(tile.normRect.top());
        const int y1 = mapToPixelY(tile.normRect.bottom());
        Q_ASSERT(x0 <= x1 && y0 <= y1);

        const auto h0 = horizontalVertices_.lower_bound(x0);
        const auto h1 = horizontalVertices_.lower_bound(x1);
        for (auto p = h0; p != h1; ++p) {
            p->second.push_back({ y0, static_cast<int>(i), 0, p == h0 });
        }

        const auto v0 = verticalVertices_.lower_bound(y0);
        const auto v1 = verticalVertices_.lower_bound(y1);
        for (auto p = v0; p != v1; ++p) {
            p->second.push_back({ x0, static_cast<int>(i), 0, p == v0 });
        }
    }

    // Sort vertices and insert terminators for convenience.
    for (auto &[x, vertices] : horizontalVertices_) {
        std::sort(vertices.begin(), vertices.end(),
                  [](const auto &a, const auto &b) { return a.pixelPos < b.pixelPos; });
        vertices.push_back({ mapToPixelY(1.0), -1, 0, false });
    }
    for (auto &[y, vertices] : verticalVertices_) {
        std::sort(vertices.begin(), vertices.end(),
                  [](const auto &a, const auto &b) { return a.pixelPos < b.pixelPos; });
        vertices.push_back({ mapToPixelX(1.0), -1, 0, false });
    }

    // Calculate spans of handles per axis.
    const auto calculateHandleSpan = [](std::map<int, std::vector<Vertex>> &vertices) {
        if (vertices.empty())
            return; // in case we allowed empty tiles.
        // First line should have no handle, so skipped.
        auto line0 = vertices.cbegin();
        for (auto line1 = std::next(vertices.begin()); line1 != vertices.end(); ++line0, ++line1) {
            auto v0s = line0->second.begin();
            auto v1s = line1->second.begin();
            Q_ASSERT(v0s != line0->second.end());
            Q_ASSERT(v1s != line1->second.end());
            auto v0p = std::next(v0s);
            auto v1p = std::next(v1s);
            while (v0p != line0->second.end() && v1p != line1->second.end()) {
                // Handle can be isolated if two vertices of the adjacent lines meet.
                if (v0p->pixelPos == v1p->pixelPos) {
                    v1s->handleSpan = static_cast<int>(v1p - v1s);
                    v0s = v0p;
                    v1s = v1p;
                    ++v0p;
                    ++v1p;
                } else if (v0p->pixelPos < v1p->pixelPos) {
                    ++v0p;
                } else {
                    ++v1p;
                }
            }
            Q_ASSERT(v0p == line0->second.end() && v1p == line1->second.end());
        }
    };
    calculateHandleSpan(horizontalVertices_);
    calculateHandleSpan(verticalVertices_);
}

void FlexTiler::resizeTiles()
{
    for (const auto &[x, vertices] : horizontalVertices_) {
        Q_ASSERT(!vertices.empty());
        for (size_t i = 0; i < vertices.size() - 1; ++i) {
            const auto &v0 = vertices.at(i);
            if (v0.tileIndex < 0 || !v0.primary)
                continue;
            if (auto &item = tiles_.at(static_cast<size_t>(v0.tileIndex)).item) {
                const qreal m = verticalHandleHeight_;
                const auto &v1 = vertices.at(i + 1);
                item->setY(static_cast<qreal>(v0.pixelPos) + m);
                item->setHeight(static_cast<qreal>(v1.pixelPos - v0.pixelPos) - m);
            }
            if (auto &item = tiles_.at(static_cast<size_t>(v0.tileIndex)).horizontalHandleItem) {
                const qreal m = verticalHandleHeight_;
                const auto &v1 = vertices.at(i + static_cast<size_t>(v0.handleSpan));
                item->setVisible(v0.handleSpan > 0);
                item->setX(static_cast<qreal>(x));
                item->setY(static_cast<qreal>(v0.pixelPos) + m);
                item->setWidth(horizontalHandleWidth_);
                item->setHeight(static_cast<qreal>(v1.pixelPos - v0.pixelPos) - m);
            }
        }
    }

    for (const auto &[y, vertices] : verticalVertices_) {
        Q_ASSERT(!vertices.empty());
        for (size_t i = 0; i < vertices.size() - 1; ++i) {
            const auto &v0 = vertices.at(i);
            if (v0.tileIndex < 0 || !v0.primary)
                continue;
            if (auto &item = tiles_.at(static_cast<size_t>(v0.tileIndex)).item) {
                const qreal m = horizontalHandleWidth_;
                const auto &v1 = vertices.at(i + 1);
                item->setX(static_cast<qreal>(v0.pixelPos) + m);
                item->setWidth(static_cast<qreal>(v1.pixelPos - v0.pixelPos) - m);
            }
            if (auto &item = tiles_.at(static_cast<size_t>(v0.tileIndex)).verticalHandleItem) {
                const qreal m = horizontalHandleWidth_;
                const auto &v1 = vertices.at(i + static_cast<size_t>(v0.handleSpan));
                item->setVisible(v0.handleSpan > 0);
                item->setX(static_cast<qreal>(v0.pixelPos) + m);
                item->setY(static_cast<qreal>(y));
                item->setWidth(static_cast<qreal>(v1.pixelPos - v0.pixelPos) - m);
                item->setHeight(verticalHandleHeight_);
            }
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
