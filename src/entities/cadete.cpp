#include "cadete.h"
#include "proyectil.h"
#include <QPainter>

Cadete::Cadete(qreal r, qreal x, qreal y, bool esJugador, qreal vida)
    : FuerzaArmada(r, esJugador, vida)
{
    setPos(x, y);

    // Inicialización de arma
    municionActual = 0;
    municionMaxima = 0;
    disparando     = false;

    // Munición inicial para el jugador
    if (esJugador) {
        definirMunicion(30, 30);   // 30/30 balas de inicio (ajusta al gusto)
    }
}

void Cadete::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!tieneSprite || sprite.isNull()) {
        p->setPen(Qt::NoPen);
        p->setBrush(jugador ? Qt::blue : Qt::green);
        p->drawEllipse(FuerzaArmada::boundingRect());
        return;
    }

    Vector2D d = getDireccion();
    if (d.magnitud2() == 0.0) d = Vector2D(0, -1);

    qreal angDeg = qRadiansToDegrees(qAtan2(d.y(), d.x()));
    angDeg -= 90.0;

    QRectF target = FuerzaArmada::boundingRect();

    p->save();
    p->setRenderHint(QPainter::SmoothPixmapTransform, false);

    p->translate(0, 0);
    p->rotate(angDeg);

    p->drawPixmap(target, sprite, sprite.rect());

    p->restore();
}

QRectF Cadete::boundingRect() const
{
    QRectF r = FuerzaArmada::boundingRect();

    const qreal m = 6.0;
    return r.adjusted(-m, -m, m, m);
}

void Cadete::definirMunicion(int cantidad, int maximo)
{
    if (cantidad < 0) cantidad = 0;

    municionActual = cantidad;

    if (maximo < 0)
        municionMaxima = cantidad;
    else
        municionMaxima = maximo;
}

void Cadete::recargar()
{
    if (municionMaxima <= 0) {
        return;
    }

    municionActual = municionMaxima;
}

void Cadete::setDisparando(bool activo)
{
    disparando = activo;
}

bool Cadete::estaDisparando() const
{
    return disparando;
}

bool Cadete::tieneMunicion() const
{
    return (municionActual > 0);
}

bool Cadete::consumirBala()
{
    if (municionActual <= 0)
        return false;

    --municionActual;
    return true;
}

int Cadete::getBalas() const
{
    return municionActual;
}

int Cadete::getBalasMax() const
{
    return municionMaxima;
}


void Cadete::recibirImpacto(Proyectil* p)
{
    if (!p) return;

    const bool proyectilDeJugador = p->esDeJugador();

    if (proyectilDeJugador && !this->jugador) {
        recibirDanio(p->getDaño());
    } else if (!proyectilDeJugador && this->esJugador()) {
        recibirDanio(p->getDaño());
    }
}

bool Cadete::esJugador() const
{
    return jugador;
}

void Cadete::setSprite(const QString& path)
{
    QPixmap tmp(path);
    if (!tmp.isNull()) {
        sprite = tmp;
        tieneSprite = true;
        update();
    } else {
        tieneSprite = false;
        sprite = QPixmap();
        update();
    }
}
