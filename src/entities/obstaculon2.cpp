#include "obstaculon2.h"

Obstaculon2::Obstaculon2()
    : m_position(0.0, 0.0)
{
    // Hitbox inicial sencilla: rectángulo tipo roca
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
