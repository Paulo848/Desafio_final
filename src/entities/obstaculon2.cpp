#include "obstaculon2.h"

Obstaculon2::Obstaculon2()
    : m_position(0.0, 0.0),
    m_spriteId(0)
{
    QVector<QPointF> puntos;
    puntos << QPointF(-15, -15)
           << QPointF( 15, -15)
           << QPointF( 15,  15)
           << QPointF(-15,  15);
    m_hitbox.setLocalPoints(puntos);
}

void Obstaculon2::setPosition(const QPointF &pos)
{
    m_position = pos;
}

QPointF Obstaculon2::position() const
{
    return m_position;
}

Hitbox &Obstaculon2::hitbox()
{
    return m_hitbox;
}

const Hitbox &Obstaculon2::hitbox() const
{
    return m_hitbox;
}

void Obstaculon2::setSpriteId(int id)
{
    m_spriteId = id;
}

int Obstaculon2::spriteId() const
{
    return m_spriteId;
}
