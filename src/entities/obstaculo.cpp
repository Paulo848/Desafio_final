#include "obstaculo.h"

// =========================
//    CIRCULAR
// =========================
Obstaculo::Obstaculo(qreal _radio)
    : forma(FormaObstaculo::Circulo),
    radio(_radio),
    ancho(0),
    alto(0)
{
    bounds = QRectF(-radio, -radio, 2*radio, 2*radio);
    setPos(0, 0); // Posición se ajusta externamente
}


// =========================
//    RECTANGULAR
// =========================
Obstaculo::Obstaculo(qreal _ancho, qreal _alto)
    : forma(FormaObstaculo::Rectangulo),
    radio(0),
    ancho(_ancho),
    alto(_alto)
{
    bounds = QRectF(-ancho/2.0, -alto/2.0, ancho, alto);
    setPos(0, 0);
}


// =========================
//    SHAPE (colisión real)
// =========================
QPainterPath Obstaculo::shape() const
{
    QPainterPath path;
    if (forma == FormaObstaculo::Circulo)
        path.addEllipse(bounds);
    else
        path.addRect(bounds);

    return path;
}


// =========================
//       DIBUJO
// =========================
void Obstaculo::paint(QPainter *p,
                      const QStyleOptionGraphicsItem *,
                      QWidget *)
{
    p->setPen(Qt::NoPen);

    if (forma == FormaObstaculo::Circulo) {
        p->setBrush(QColor(Qt::red));  // gris piedra
        p->drawEllipse(bounds);
    }
    else {
        p->setBrush(QColor(Qt::yellow));   // marrón tierra/madera
        p->drawRect(bounds);
    }
}
