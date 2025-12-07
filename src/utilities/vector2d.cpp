#include "vector2d.h"

// this = dirección forward (jugador -> mouse), NO tiene por qué venir normalizado
Vector2D Vector2D::rotCanonicoY(const Vector2D &local) const
{
    // forward = this normalizado
    Vector2D forward = this->normalizado();

    // right = perpendicular a forward (hacia la derecha)
    Vector2D right(forward.y(), -forward.x());

    // world = combinacion lineal: local.x * right + local.y * forward
    return right * local.x() + forward * local.y();
}

Vector2D Vector2D::getNormal() const{

    Vector2D out(0.0,0.0);
    out.setX(this->y());
    out.setY(this->x()*-1.0);

    return out;

}
