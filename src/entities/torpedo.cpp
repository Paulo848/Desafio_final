#include "torpedo.h"

Torpedo::Torpedo()
    : m_position(0.0, 0.0),
    m_velocidad(6.0),  // Velocidad rápida hacia adelante
    m_activo(true)
{
    // Hitbox del torpedo (pequeña y alargada como un proyectil)
    QVector<QPointF> puntos;
    puntos << QPointF(-5, -4)
           << QPointF( 20, -4)
           << QPointF( 20,  4)
           << QPointF(-5,  4);
    m_hitbox.setLocalPoints(puntos);
}

void Torpedo::setPosition(const QPointF &pos)
{
    m_position = pos;
}

QPointF Torpedo::position() const
{
    return m_position;
}

void Torpedo::actualizar()
{
    if (m_activo) {
        // Mover hacia adelante (aumentar X en el mundo lógico)
        m_position.setX(m_position.x() + m_velocidad);
    }
}

bool Torpedo::estaActivo() const
{
    return m_activo;
}

void Torpedo::desactivar()
{
    m_activo = false;
}

Hitbox &Torpedo::hitbox()
{
    return m_hitbox;
}

const Hitbox &Torpedo::hitbox() const
{
    return m_hitbox;
}
