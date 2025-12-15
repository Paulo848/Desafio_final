#ifndef TORPEDO_H
#define TORPEDO_H

#include <QPointF>
#include "hitbox.h"
#include <QPixmap>

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

    // Sprite
    void setSprite(const QPixmap &sprite);

    // Obtiene el sprite actual
    const QPixmap& getSprite() const;

    // Verifica si tiene sprite cargado
    bool tieneSprite() const;

private:
    QPointF m_position;
    Hitbox m_hitbox;
    qreal m_velocidad;  // Velocidad del torpedo
    bool m_activo;      // Si el torpedo está en juego
    QPixmap m_sprite;
    bool m_tieneSprite;
};

#endif // TORPEDO_H
