#pragma once
#include <QPointF>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <memory>
#include <tuple>
#include "flextilelayouter.h"

class FlexTilerAttached;

class FlexTiler : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(QQmlComponent *horizontalHandle READ horizontalHandle WRITE setHorizontalHandle
                       NOTIFY horizontalHandleChanged FINAL)
    Q_PROPERTY(QQmlComponent *verticalHandle READ verticalHandle WRITE setVerticalHandle NOTIFY
                       verticalHandleChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QQuickItem *currentItem READ currentItem NOTIFY currentItemChanged)
    QML_ATTACHED(FlexTilerAttached)
    QML_ELEMENT

public:
    explicit FlexTiler(QQuickItem *parent = nullptr);
    ~FlexTiler() override;

    static FlexTilerAttached *qmlAttachedProperties(QObject *object);

    QQmlComponent *delegate() { return tileDelegate_; }
    void setDelegate(QQmlComponent *delegate);

    QQmlComponent *horizontalHandle() { return horizontalHandle_; }
    void setHorizontalHandle(QQmlComponent *handle);

    QQmlComponent *verticalHandle() { return verticalHandle_; }
    void setVerticalHandle(QQmlComponent *handle);

    int count() const;
    int currentIndex() const { return currentIndex_; }
    void setCurrentIndex(int index);
    QQuickItem *currentItem() const;
    Q_INVOKABLE QQuickItem *itemAt(int index) const;

    Q_INVOKABLE void split(int index, Qt::Orientation orientation, int count = 2);
    Q_INVOKABLE void close(int index);

signals:
    void delegateChanged();
    void horizontalHandleChanged();
    void verticalHandleChanged();
    void countChanged();
    void currentIndexChanged();
    void currentItemChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void updatePolish() override;

private:
    using KeyRect = FlexTileLayouter::KeyRect;
    using Tile = FlexTileLayouter::Tile;
    using UniqueItemPtr = FlexTileLayouter::UniqueItemPtr;

    void recreateTiles();
    Tile createTile(const KeyRect &normRect, int index);
    std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>> createTileItem(int index);
    std::tuple<UniqueItemPtr, std::unique_ptr<QQmlContext>>
    createHandleItem(QQmlComponent *component);
    void updateTileIndices(int from);
    void resetCurrentIndex(int index);

    QRectF extendedOuterPixelRect() const;

    FlexTileLayouter layouter_;
    QPointer<QQmlComponent> tileDelegate_ = nullptr;
    QPointer<QQmlComponent> horizontalHandle_ = nullptr;
    QPointer<QQmlComponent> verticalHandle_ = nullptr;
    qreal horizontalHandlePixelWidth_ = 0.0;
    qreal verticalHandlePixelHeight_ = 0.0;
    QPointF movingHandleGrabPixelOffset_;
    int currentIndex_ = 0; // should have at least one tile
};

class FlexTilerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FlexTiler *tiler READ tiler NOTIFY tilerChanged FINAL)
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY minimumWidthChanged
                       FINAL)
    Q_PROPERTY(qreal minimumHeight READ minimumHeight WRITE setMinimumHeight NOTIFY
                       minimumHeightChanged FINAL)
    Q_PROPERTY(bool closable READ closable NOTIFY closableChanged FINAL)

public:
    explicit FlexTilerAttached(QObject *parent = nullptr);

    FlexTiler *tiler() const { return tiler_; }
    void setTiler(FlexTiler *tiler);

    int index() const { return index_; }
    void setIndex(int index);

    qreal minimumWidth() const { return minimumWidth_; }
    void setMinimumWidth(qreal width);

    qreal minimumHeight() const { return minimumHeight_; }
    void setMinimumHeight(qreal height);

    bool closable() const { return closable_; }
    void setClosable(bool closable);

signals:
    void tilerChanged();
    void indexChanged();
    void minimumWidthChanged();
    void minimumHeightChanged();
    void closableChanged();

private:
    void requestPolish();

    QPointer<FlexTiler> tiler_ = nullptr;
    int index_ = -1;
    qreal minimumWidth_ = 0.0;
    qreal minimumHeight_ = 0.0;
    bool closable_ = false;
};
