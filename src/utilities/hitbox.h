#ifndef HITBOX_H
#define HITBOX_H

#include <QPointF>
#include <QVector>

class Hitbox
{
public:
    Hitbox();

    // Define los puntos en coordenadas LOCALES al objeto
    void setLocalPoints(const QVector<QPointF> &points);
    const QVector<QPointF> &localPoints() const;

    // Puntos transformados al mundo 2D lógico usando la posición del objeto
    QVector<QPointF> worldPoints(const QPointF &objectPosition) const;

    // Colisión simple (por ahora usaremos un AABB aproximado)
    bool intersects(const Hitbox &other,
                    const QPointF &thisPos,
                    const QPointF &otherPos) const;

    void setColliding(bool value);
    bool isColliding() const;

private:
    QVector<QPointF> m_localPoints;
    bool m_colliding;
};

#endif // HITBOX_H
