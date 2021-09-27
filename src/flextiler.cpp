#include <QtQml>
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
    tiles_.push_back({ { 0.0, 0.0, 1.0, 1.0 }, {}, {} });
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

void FlexTiler::recreateTiles()
{
    for (size_t i = 0; i < tiles_.size(); ++i) {
        tiles_.at(i) = createTile(tiles_.at(i).normRect, static_cast<int>(i));
    }
    polish();
}

auto FlexTiler::createTile(const QRectF &normRect, int index) -> Tile
{
    if (!tileDelegate_)
        return { normRect, {}, {} };

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
        return { normRect, std::move(item), std::move(context) };
    } else {
        qmlWarning(this) << "tile component does not create an item";
        delete obj;
        return { normRect, {}, {} };
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

void FlexTiler::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    polish();
}

void FlexTiler::updatePolish()
{
    // TODO
    if (auto &item = tiles_.at(0).item) {
        item->setSize(size());
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
