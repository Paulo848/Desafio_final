#ifndef OBSTACULON2_H
#define OBSTACULON2_H

#include <QPointF>
#include "hitbox.h"

class Obstaculon2
{
public:
    Obstaculon2();

    void setPosition(const QPointF &pos);
    QPointF position() const;

    Hitbox &hitbox();
    const Hitbox &hitbox() const;

private:
    QPointF m_position;
    Hitbox m_hitbox;
};

#endif // OBSTACULON2_H
