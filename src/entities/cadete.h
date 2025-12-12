#ifndef CADETE_H
#define CADETE_H

#include "fuerzaarmada.h"

class Cadete : public FuerzaArmada
{
private:
    // --- Estado de arma / munición (para IA de oleadas) ---
    int  municionActual = 0;
    int  municionMaxima = 0;
    bool disparando     = false;

public:
    Cadete(qreal r = 10.0,
           qreal x = 0.0,
           qreal y = 0.0,
           bool esJugador = false,
           qreal vida = 10);

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;
    void recibirImpacto(Proyectil* p) override;
    bool esJugador() const override;

    // ============================
    //  Control de disparo / munición
    //  (usado por OleadaCadetes)
    // ============================

    // Define la munición actual y, opcionalmente, la máxima.
    // Si maximo < 0, se toma maximo = cantidad.
    void definirMunicion(int cantidad, int maximo = -1);

    // Recarga el cargador al máximo permitido.
    void recargar();

    // Activa o desactiva el modo "disparando".
    void setDisparando(bool activo);

    // Indica si este cadete está en modo de disparar (flag alto nivel).
    bool estaDisparando() const;

    // ¿Tiene al menos 1 bala en el cargador?
    bool tieneMunicion() const;

    // Intenta consumir una bala. Devuelve true si pudo disparar.
    bool consumirBala();

    // Getters usados por el HUD (Nivel)
    int getBalas() const;
    int getBalasMax() const;


};

#endif // CADETE_H
