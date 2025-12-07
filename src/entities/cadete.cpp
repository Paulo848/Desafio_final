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
}

void Cadete::paint(QPainter *painter,
                   const QStyleOptionGraphicsItem *,
                   QWidget *)
{
    // color distinto si es jugador o enemigo, por ejemplo
    painter->setPen(Qt::NoPen);
    painter->setBrush(jugador ? Qt::green : Qt::blue);
    painter->drawEllipse(boundingRect());
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

void Cadete::recibirImpacto(Proyectil* p)
{
    if (!p) return;

    // Solo sufro daño si el proyectil viene del bando contrario
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
