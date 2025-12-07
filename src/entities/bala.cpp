#include "bala.h"
#include "fuerzaarmada.h"
#include "proyectil.h"

Bala::Bala(FuerzaArmada *emisor,
           const Vector2D &dir)
    : Proyectil(emisor,
                dir,       // dirección
                12.0,      // velocidad
                3.0,       // radio
                10)        // daño
{
    setZValue(-900);
}

void Bala::aplicarColision(FuerzaArmada *objetivo)
{
    // Resta vida
    objetivo->recibirImpacto(this);

    // Aquí podrías meter animaciones / efectos
}

void Bala::aplicarImpacto(FuerzaArmada *obj)
{
    if (esDeJugador() && obj->esJugador())
        return; // friendly fire off

    if (!esDeJugador() && !obj->esJugador())
        return; // enemigos entre sí no se pegan

    obj->recibirImpacto(this);
    muerto = true;
}

bool Bala::esDeJugador() const{
    bool es = emisor->esJugador() ? true : false;
    return  es;
}
