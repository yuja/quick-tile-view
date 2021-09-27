#pragma once
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QRectF>
#include <memory>
#include <vector>

class FlexTilerAttached;

class FlexTiler : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    QML_ATTACHED(FlexTilerAttached)
    QML_ELEMENT

public:
    explicit FlexTiler(QQuickItem *parent = nullptr);
    ~FlexTiler() override;

    static FlexTilerAttached *qmlAttachedProperties(QObject *object);

    QQmlComponent *delegate() { return tileDelegate_; }
    void setDelegate(QQmlComponent *delegate);

    int count() const;
    Q_INVOKABLE QQuickItem *itemAt(int index) const;

signals:
    void delegateChanged();
    void countChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void updatePolish() override;

private:
    class ItemDeleter
    {
    public:
        void operator()(QQuickItem *item) const;
    };

    struct Tile
    {
        QRectF normRect;
        std::unique_ptr<QQuickItem, ItemDeleter> item; // may be nullptr
        std::unique_ptr<QQmlContext> context; // may be nullptr if !item
    };

    void recreateTiles();
    Tile createTile(const QRectF &normRect, int index);

    std::vector<Tile> tiles_;
    QPointer<QQmlComponent> tileDelegate_ = nullptr;
};

class FlexTilerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FlexTiler *tiler READ tiler NOTIFY tilerChanged FINAL)
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)

public:
    explicit FlexTilerAttached(QObject *parent = nullptr);

    FlexTiler *tiler() const { return tiler_; }
    void setTiler(FlexTiler *tiler);

    int index() const { return index_; }
    void setIndex(int index);

signals:
    void tilerChanged();
    void indexChanged();

private:
    QPointer<FlexTiler> tiler_ = nullptr;
    int index_ = -1;
};
