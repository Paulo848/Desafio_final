#include "barco.h"

Barco::Barco()
    : m_position(0.0, 0.0)
{
    // Hitbox inicial sencilla: rectángulo centrado en (0,0)
    QVector<QPointF> puntos;
    puntos << QPointF(-20, -10)
           << QPointF( 20, -10)
           << QPointF( 20,  10)
           << QPointF(-20,  10);
    m_hitbox.setLocalPoints(puntos);
}

void Barco::setPosition(const QPointF &pos)
{
    m_position = pos;
}

QPointF Barco::position() const
{
    return m_position;
}

Hitbox &Barco::hitbox()
{
    return m_hitbox;
}

const Hitbox &Barco::hitbox() const
{
    return m_hitbox;
}
