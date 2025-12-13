#ifndef MISIL_H
#define MISIL_H

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPainter>
#include "fuerzaarmada.h"

class Avion;

class Misil: public Proyectil
{
public:
    Misil(FuerzaArmada* duenho, const Vector2D &dir);
    ~Misil();
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void avanzar() override;
    void aplicarColision(FuerzaArmada *objetivo) override;
    bool esDeJugador() const override;
    void aplicarImpacto(FuerzaArmada* obj) override;
    void Colision_Avion();

    void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget);
    bool operator==(Misil* other);
    bool operator!=(Misil* other);

private:
    QPixmap sprite;
    void Colision_Balas(Misil* other);
};

#endif // MISIL_H
