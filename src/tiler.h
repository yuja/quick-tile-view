#pragma once
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <QSizeF>
#include <memory>
#include <tuple>
#include <vector>

class TilerAttached;

class Tiler : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(QQmlComponent *horizontalHandle READ horizontalHandle WRITE setHorizontalHandle
                       NOTIFY horizontalHandleChanged FINAL)
    Q_PROPERTY(QQmlComponent *verticalHandle READ verticalHandle WRITE setVerticalHandle NOTIFY
                       verticalHandleChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    QML_ATTACHED(TilerAttached)
    QML_ELEMENT

public:
    explicit Tiler(QQuickItem *parent = nullptr);
    ~Tiler() override;

    static TilerAttached *qmlAttachedProperties(QObject *object);

    QQmlComponent *delegate() { return tileDelegate_; }
    void setDelegate(QQmlComponent *delegate);

    QQmlComponent *horizontalHandle() { return horizontalHandle_; }
    void setHorizontalHandle(QQmlComponent *handle);

    QQmlComponent *verticalHandle() { return verticalHandle_; }
    void setVerticalHandle(QQmlComponent *handle);

    int count() const;
    Q_INVOKABLE QQuickItem *itemAt(int tileIndex) const;

    Q_INVOKABLE void split(int tileIndex, Qt::Orientation orientation);
    Q_INVOKABLE void close(int tileIndex);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void updatePolish() override;

signals:
    void delegateChanged();
    void horizontalHandleChanged();
    void verticalHandleChanged();
    void countChanged();

private:
    class ItemDeleter
    {
    public:
        void operator()(QQuickItem *item) const;
    };

    struct Tile
    {
        std::unique_ptr<QQuickItem, ItemDeleter> item; // may be nullptr
        std::unique_ptr<QQmlContext> context; // may be nullptr if !item
    };

    struct Band
    {
        int index; // >=0: tile[i], <0: splitMap[-i]
        qreal position;
        std::unique_ptr<QQuickItem, ItemDeleter> handleItem; // may be nullptr
        std::unique_ptr<QQmlContext> handleContext; // may be nullptr if !item
    };

    struct Split
    {
        Qt::Orientation orientation;
        std::vector<Band> bands;
        QRectF outerRect; // cache updated by resizeTiles()
        QSizeF minimumSize; // cache updated by accumulateTiles()
    };

    void recreateTiles();
    Tile createTile(int index);
    void recreateHandles(Qt::Orientation orientation);
    Band createBand(int index, qreal position, Qt::Orientation orientation);
    std::tuple<int, int> findSplitBandByIndex(int index) const;
    std::tuple<int, int> findSplitBandByHandleItem(const QQuickItem *item) const;
    QSizeF minimumSizeByIndex(int index) const;
    bool unlinkTileByIndex(Split &split, int index, int depth);
    void cleanTrailingEmptySplits();
    void moveSplitBand(int splitIndex, int bandIndex, const QPointF &itemPos);
    void accumulateTiles(int splitIndex, int depth);
    void resizeTiles(int splitIndex, const QRectF &outerRect, int depth);

    std::vector<Tile> tiles_;
    std::vector<Split> splitMap_;
    QPointer<QQmlComponent> tileDelegate_ = nullptr;
    QPointer<QQmlComponent> horizontalHandle_ = nullptr;
    QPointer<QQmlComponent> verticalHandle_ = nullptr;
    int movingSplitIndex_ = -1;
    int movingBandIndex_ = -1;
    QPointF movingSplitBandGrabOffset_;
};

class TilerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Tiler *tiler READ tiler NOTIFY tilerChanged FINAL)
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY minimumWidthChanged
                       FINAL)
    Q_PROPERTY(qreal minimumHeight READ minimumHeight WRITE setMinimumHeight NOTIFY
                       minimumHeightChanged FINAL)

public:
    explicit TilerAttached(QObject *parent = nullptr);

    Tiler *tiler() const { return tiler_; }
    void setTiler(Tiler *tiler);

    int index() const { return index_; }
    void setIndex(int index);

    qreal minimumWidth() const { return minimumWidth_; }
    void setMinimumWidth(qreal width);

    qreal minimumHeight() const { return minimumHeight_; }
    void setMinimumHeight(qreal height);

signals:
    void tilerChanged();
    void indexChanged();
    void minimumWidthChanged();
    void minimumHeightChanged();

private:
    void requestPolish();

    QPointer<Tiler> tiler_ = nullptr;
    int index_ = -1;
    qreal minimumWidth_ = 0.0;
    qreal minimumHeight_ = 0.0;
};
