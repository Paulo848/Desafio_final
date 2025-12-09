#ifndef MISIL_H
#define MISIL_H

#include <QGraphicsItem>
#include <QPainter>
#include "fuerzaarmada.h"

class Misil: public Proyectil
{
public:
    Misil(qreal x, qreal y);
    ~Misil();

    qreal getx() const;
    qreal gety() const;
    qreal getvelocidad() const;
    short int getdamage() const;
    bool getcrashed() const;
    void setcrashed(bool estado);
    QRectF boundingRect() const override;
    void avanzar() override;
    void aplicarColision(FuerzaArmada *objetivo) override;
    bool esDeJugador() const override;
    void aplicarImpacto(FuerzaArmada* obj) override;
    bool Colision_Balas(Misil* other);

    // Dibujo común para proyectil esférico
    void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget);
    void Desplazar(FuerzaArmada* emisor);
    bool operator==(Misil* other);
    bool operator!=(Misil* other);

private:
    float y0; // velocidad inicial vertical
    float a;  // gravedad
    float t;
    qreal x;
    qreal y;
    qreal velocidad = 5;
    short int damage = 200;
    bool crashed = false;
};

#endif // MISIL_H
