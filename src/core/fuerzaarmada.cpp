#include "fuerzaarmada.h"

FuerzaArmada::FuerzaArmada(qreal r, bool EsJugador, qreal vida)
    : direccion(0.0, -1.0), radio(r), vida(vida), velocidad(3.0), muerto(false), jugador(EsJugador)
{
    setPos(0, 0); // posición base
}

QRectF FuerzaArmada::boundingRect() const
{
    return QRectF(-radio, -radio, 2*radio, 2*radio);
}

QPainterPath FuerzaArmada::shape() const{
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}

void FuerzaArmada::paint(QPainter *painter,
                         const QStyleOptionGraphicsItem *option,
                         QWidget *widget)
{
    // Por defecto, dibuja un círculo gris.
    painter->setBrush(Qt::gray);
    painter->drawEllipse(boundingRect());
}

void FuerzaArmada::recibirDanio(int d)
{
    vida -= d;
    if (vida <= 0) {
        vida = 0;
        morir();
    }
}

void FuerzaArmada::morir()
{
    if (muerto) return;

    muerto = true;

    // Opcional: desactivar visualmente el item.
    setEnabled(false);
    setVisible(false);

    // La eliminación real del objeto (delete) la seguirá
    // manejando el Nivel, como ya lo estás haciendo.
}
