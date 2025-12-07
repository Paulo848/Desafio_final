#include "proyectil.h"
#include "fuerzaarmada.h"
#include <QPainter>
#include <QGraphicsScene>
#include "obstaculo.h"

Proyectil::Proyectil(FuerzaArmada *emitidoPor,
                     const Vector2D &dir,
                     qreal vel,
                     qreal r,
                     int dmg)
    : direccion(dir.normalizado()),
    velocidad(vel),
    radio(r),
    daño(dmg),
    emisor(emitidoPor),
    bounds(-r, -r, 2*r, 2*r)
{
    setPos( emisor->scenePos() );   // Inicia donde está el emisor
    muerto = false;
    vidaTicks = 250;   // vive 180 ciclos del timer (~3s si el timer es 16ms)
}

QPainterPath Proyectil::shape() const
{
    QPainterPath p;
    p.addEllipse(bounds);
    return p;
}

void Proyectil::avanzar()
{
    if (muerto) return;

    // Avance normal
    Vector2D delta = direccion * velocidad;
    setPos(pos().x() + delta.x(),
           pos().y() + delta.y());

    // Vida por tiempo:
    vidaTicks--;
    if (vidaTicks <= 0) {
        muerto = true;
        return;
    }

    // Limitar por escena:
    if (!scene()->sceneRect().contains(scenePos())) {
        muerto = true;
        return;
    }

    // Colisiones:

    auto cols = collidingItems();
    for (auto item : cols)
    {
        if (item == emisor) continue;

        FuerzaArmada* fa = dynamic_cast<FuerzaArmada*>(item);
        if (fa) {
            //qDebug() << "Bala choco con FuerzaArmada:" << fa;
            if(fa->esJugador() && !emisor->esJugador()){
                aplicarImpacto(fa);
                muerto = true;
            }
            if(!fa->esJugador() && emisor->esJugador()){
                aplicarImpacto(fa);
                muerto = true;
            }
            return;
        }

        Obstaculo* obs = dynamic_cast<Obstaculo*>(item);
        if (obs) {
            //qDebug() << "Bala chocó con Obstaculo:" << obs;
            muerto = true;
            return;
        }
    }

}


void Proyectil::paint(QPainter *p,
                      const QStyleOptionGraphicsItem *,
                      QWidget *)
{
    p->setBrush(Qt::yellow);
    p->setPen(Qt::NoPen);
    p->drawEllipse(bounds);
}
