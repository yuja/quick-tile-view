#include <QCursor>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QtQml>
#include <algorithm>
#include "flextilelayouter.h"
#include "flextiler.h"

namespace {
FlexTilerAttached *tileAttached(const QQuickItem *item)
{
    if (!item)
        return nullptr;
    return qobject_cast<FlexTilerAttached *>(qmlAttachedPropertiesObject<FlexTiler>(item));
}

constexpr qreal snapPixelSize = 5.0;
}

FlexTiler::FlexTiler(QQuickItem *parent) : QQuickItem(parent)
{
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setCursor(Qt::ArrowCursor);
    setFlag(ItemIsFocusScope);
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
    for (size_t i = 0; i < layouter_.count(); ++i) {
        auto &tile = layouter_.tileAt(i);
        tile = createTile(tile.normRect, static_cast<int>(i));
    }
    resetCurrentIndex(currentIndex_);
    polish();
}

auto FlexTiler::createTile(const KeyRect &normRect, int index) -> Tile
{
    auto [item, context] = createTileItem(index);
    auto [hHandleItem, hHandleContext] = createHandleItem(horizontalHandle_);
    auto [vHandleItem, vHandleContext] = createHandleItem(verticalHandle_);
    // Apply identical width/height to all handles to make the layouter simple.
    horizontalHandlePixelWidth_ = hHandleItem ? hHandleItem->implicitWidth() : 0.0;
    verticalHandlePixelHeight_ = vHandleItem ? vHandleItem->implicitHeight() : 0.0;
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

auto FlexTiler::createHandleItem(QQmlComponent *component)
        -> std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>>
{
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
        return { std::move(item), std::move(context) };
    } else {
        qmlWarning(this) << "handle component does not create an item";
        delete obj;
        return {};
    }
}

void FlexTiler::updateTileIndices(int from)
{
    for (size_t i = static_cast<size_t>(from); i < layouter_.count(); ++i) {
        if (auto *a = tileAttached(layouter_.tileAt(i).item.get())) {
            a->setIndex(static_cast<int>(i));
        }
    }
}

int FlexTiler::count() const
{
    return static_cast<int>(layouter_.count());
}

void FlexTiler::setCurrentIndex(int index)
{
    if (currentIndex_ == index)
        return;
    currentIndex_ = index;
    emit currentIndexChanged();
    emit currentItemChanged();
}

void FlexTiler::resetCurrentIndex(int index)
{
    if (currentIndex_ != index) {
        currentIndex_ = index;
        emit currentIndexChanged();
    }
    emit currentItemChanged();
}

QQuickItem *FlexTiler::currentItem() const
{
    return currentIndex_ >= 0 ? itemAt(currentIndex_) : nullptr;
}

QQuickItem *FlexTiler::itemAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(layouter_.count())) {
        qmlWarning(this) << "tile index out of range:" << index;
        return nullptr;
    }

    return layouter_.tileAt(static_cast<size_t>(index)).item.get();
}

void FlexTiler::split(int index, Qt::Orientation orientation, int count)
{
    if (index < 0 || index >= static_cast<int>(layouter_.count())) {
        qmlWarning(this) << "tile index out of range:" << index;
        return;
    }
    if (count < 2)
        return;

    const int shiftedCurrentIndex = currentIndex_ + (index < currentIndex_ ? count - 1 : 0);
    std::vector<Tile> newTiles;
    for (int i = 1; i < count; ++i) {
        newTiles.push_back(createTile({ 0.0, 0.0, 0.0, 0.0 }, index + i));
    }
    const auto outerRect = extendedOuterPixelRect();
    const QSizeF snapSize(snapPixelSize / outerRect.width(), snapPixelSize / outerRect.height());
    layouter_.split(static_cast<size_t>(index), orientation, std::move(newTiles), snapSize);

    updateTileIndices(index + count);
    setCurrentIndex(shiftedCurrentIndex);
    polish();
    emit countChanged();
}

void FlexTiler::close(int index)
{
    if (index < 0 || index >= static_cast<int>(layouter_.count())) {
        qmlWarning(this) << "tile index out of range:" << index;
        return;
    }

    const bool currentClosing = index == currentIndex_;
    const int shiftedCurrentIndex = currentIndex_ - static_cast<int>(index < currentIndex_);
    const int collapsedToIndex = layouter_.close(static_cast<size_t>(index));
    if (collapsedToIndex < 0) {
        qmlInfo(this) << "no collapsible tiles found for " << index;
        return;
    }

    updateTileIndices(index);
    if (currentClosing) {
        resetCurrentIndex(collapsedToIndex);
    } else {
        setCurrentIndex(shiftedCurrentIndex);
    }
    polish();
    emit countChanged();
}

void FlexTiler::hoverEnterEvent(QHoverEvent *event)
{
    updateHovered(event->position());
}

void FlexTiler::hoverMoveEvent(QHoverEvent *event)
{
    updateHovered(event->position());
}

void FlexTiler::hoverLeaveEvent(QHoverEvent *)
{
    setCursor(Qt::ArrowCursor);
}

void FlexTiler::updateHovered(const QPointF &position)
{
    const auto *item = childAt(position.x(), position.y());
    const auto [index, orientations] = layouter_.findTileByHandleItem(item);
    switch (orientations) {
    case Qt::Horizontal:
        setCursor(Qt::SplitHCursor);
        break;
    case Qt::Vertical:
        setCursor(Qt::SplitVCursor);
        break;
    case Qt::Horizontal | Qt::Vertical:
        setCursor(Qt::SizeAllCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
    }
}

void FlexTiler::mousePressEvent(QMouseEvent *event)
{
    const auto *item = childAt(event->position().x(), event->position().y());
    const auto [index, orientations] = layouter_.findTileByHandleItem(item);
    if (index < 0)
        return;

    // Determine tiles to be moved on press because the grid may change while moving
    // the selected handle.
    const bool lineThrough = event->modifiers() & Qt::ShiftModifier;
    layouter_.startMoving(static_cast<size_t>(index), orientations, lineThrough,
                          extendedOuterPixelRect(),
                          { horizontalHandlePixelWidth_, verticalHandlePixelHeight_ });
    movingHandleGrabPixelOffset_ = event->position() - item->position();
    setKeepMouseGrab(true);
}

void FlexTiler::mouseMoveEvent(QMouseEvent *event)
{
    if (!layouter_.isMoving())
        return;

    const auto outerRect = extendedOuterPixelRect();
    const auto pixelPos = event->position() - movingHandleGrabPixelOffset_;
    const QPointF normPos((pixelPos.x() - outerRect.left()) / outerRect.width(),
                          (pixelPos.y() - outerRect.top()) / outerRect.height());
    const QSizeF snapSize(snapPixelSize / outerRect.width(), snapPixelSize / outerRect.height());
    layouter_.moveTo(normPos, snapSize);

    polish();
}

void FlexTiler::mouseReleaseEvent(QMouseEvent * /*event*/)
{
    layouter_.resetMovingState();
    movingHandleGrabPixelOffset_ = {};
    setKeepMouseGrab(false);
}

void FlexTiler::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    layouter_.resetMovingState();
    polish();
}

void FlexTiler::updatePolish()
{
    layouter_.resizeTiles(extendedOuterPixelRect(),
                          { horizontalHandlePixelWidth_, verticalHandlePixelHeight_ });
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
