#ifndef TILEGENERATOR_H
#define TILEGENERATOR_H

#include <QQuickItem>

class TileGenerator : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TileGenerator(QQuickItem *parent = nullptr);
};

#endif // TILEGENERATOR_H
