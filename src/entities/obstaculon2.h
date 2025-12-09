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

    void setSpriteId(int id);
    int spriteId() const;

private:
    QPointF m_position;
    Hitbox m_hitbox;

    int m_spriteId;
};

#endif // OBSTACULON2_H
