#include "hitbox.h"
#include <algorithm>
#include <QRectF>


Hitbox::Hitbox()
    : m_colliding(false)
{
}

void Hitbox::setLocalPoints(const QVector<QPointF> &points)
{
    m_localPoints = points;
}

const QVector<QPointF> &Hitbox::localPoints() const
{
    return m_localPoints;
}

QVector<QPointF> Hitbox::worldPoints(const QPointF &objectPosition) const
{
    QVector<QPointF> result;
    result.reserve(m_localPoints.size());

    for (const QPointF &p : m_localPoints) {
        result.append(p + objectPosition);
    }

    return result;
}

bool Hitbox::intersects(const Hitbox &other,
                        const QPointF &thisPos,
                        const QPointF &otherPos) const
{
    // Si no hay puntos, no hay hitbox
    if (m_localPoints.isEmpty() || other.m_localPoints.isEmpty())
        return false;

    // Convertir a mundo
    QVector<QPointF> thisWorld = worldPoints(thisPos);
    QVector<QPointF> otherWorld = other.worldPoints(otherPos);

    // AABB simple de cada uno
    auto computeAABB = [](const QVector<QPointF> &pts) {
        qreal minX = pts.first().x();
        qreal maxX = pts.first().x();
        qreal minY = pts.first().y();
        qreal maxY = pts.first().y();

        for (const QPointF &p : pts) {
            if (p.x() < minX) minX = p.x();
            if (p.x() > maxX) maxX = p.x();
            if (p.y() < minY) minY = p.y();
            if (p.y() > maxY) maxY = p.y();
        }

        return QRectF(QPointF(minX, minY),
                      QPointF(maxX, maxY));
    };

    QRectF a = computeAABB(thisWorld);
    QRectF b = computeAABB(otherWorld);

    return a.intersects(b);
}

void Hitbox::setColliding(bool value)
{
    m_colliding = value;
}

bool Hitbox::isColliding() const
{
    return m_colliding;
}
