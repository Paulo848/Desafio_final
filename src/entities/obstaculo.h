#ifndef OBSTACULO_H
#define OBSTACULO_H

#include <QGraphicsItem>
#include <QPainter>

enum class FormaObstaculo {
    Circulo,
    Rectangulo
};

class Obstaculo : public QGraphicsItem
{
private:
    FormaObstaculo forma;

    // Para círculos
    qreal radio;

    // Para rectángulos
    qreal ancho;
    qreal alto;

    // Bounding rect precalculado
    QRectF bounds;

public:
    // --- Constructor circular ---
    Obstaculo(qreal _radio);

    // --- Constructor rectangular ---
    Obstaculo(qreal _ancho, qreal _alto);

    // Bounding rect y shape para colisiones
    QRectF boundingRect() const override { return bounds; }
    QPainterPath shape() const override;

    // Dibujar obstáculo
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    // Forma
    inline FormaObstaculo getForma() const { return forma; }
};

#endif // OBSTACULO_H
