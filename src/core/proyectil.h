#ifndef PROYECTIL_H
#define PROYECTIL_H

#include <QGraphicsItem>
#include "vector2d.h"

class FuerzaArmada;

class Proyectil : public QGraphicsItem
{
protected:
    Vector2D direccion;        // Siempre unitario
    qreal velocidad;
    qreal radio;
    int daño;
    FuerzaArmada *emisor;      // Quien dispara
    QRectF bounds;             // boundingRect precalculado

public:

    bool muerto;
    int vidaTicks;

    Proyectil(FuerzaArmada *emitidoPor,
              const Vector2D &dir,
              qreal vel,
              qreal r,
              int dmg);

    virtual ~Proyectil() {}

    QRectF boundingRect() const override { return bounds; }
    QPainterPath shape() const override;

    // El cuerpo del movimiento es igual en todos los proyectiles
    virtual void avanzar();

    // Cada tipo decide su efecto al colisionar
    virtual void aplicarColision(FuerzaArmada *objetivo) = 0;

    // Dibujo común para proyectil esférico
    virtual void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *,
                       QWidget *);

    virtual bool esDeJugador() const = 0;
    virtual void aplicarImpacto(FuerzaArmada* obj) = 0;

    inline int getDaño(){
        return daño;
    }

};

#endif
