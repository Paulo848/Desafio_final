#ifndef BARCO_H
#define BARCO_H

#include <QPointF>
#include "hitbox.h"

class Barco
{
public:
    Barco();

    void setPosition(const QPointF &pos);
    QPointF position() const;

    Hitbox &hitbox();
    const Hitbox &hitbox() const;

private:
    QPointF m_position;
    Hitbox m_hitbox;
};

#endif // BARCO_H
