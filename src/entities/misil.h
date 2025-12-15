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
    void setDireccionCaida();
    void setdañoAplicado(bool estado);
    void Colision_Balas(Misil* other);

    void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget);
    bool operator==(Misil* other);
    bool operator!=(Misil* other);

private:
    bool dañoAplicado = false;
    qreal largo;
    qreal ancho;
    QPixmap sprite;
};

#endif // MISIL_H
