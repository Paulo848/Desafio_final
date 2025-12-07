#ifndef TORPEDO_H
#define TORPEDO_H

#include <QPointF>
#include "hitbox.h"

class Torpedo
{
public:
    Torpedo();

    void setPosition(const QPointF &pos);
    QPointF position() const;

    void actualizar();  // Mover el torpedo hacia adelante
    bool estaActivo() const;
    void desactivar();

    Hitbox &hitbox();
    const Hitbox &hitbox() const;

private:
    QPointF m_position;
    Hitbox m_hitbox;
    qreal m_velocidad;  // Velocidad del torpedo
    bool m_activo;      // Si el torpedo está en juego
};

#endif // TORPEDO_H
