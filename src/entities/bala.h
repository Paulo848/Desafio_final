#ifndef BALA_H
#define BALA_H

#include "proyectil.h"

class Bala : public Proyectil
{
public:
    Bala(FuerzaArmada *emisor,
         const Vector2D &dir, int d);

    void aplicarColision(FuerzaArmada *objetivo) override;
    void aplicarImpacto(FuerzaArmada *obj) override;

    bool esDeJugador() const override;

};

#endif
