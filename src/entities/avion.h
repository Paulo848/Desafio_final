#ifndef AVION_H
#define AVION_H

#include "fuerzaarmada.h"
#include "misil.h"
#include <QTimer>
#include <QPainter>

class Avion: public FuerzaArmada
{
public:
    Avion(qreal r, qreal x, qreal y, bool bando = false, short int speed = 3);
    ~Avion();
    void Mov_derecha();
    void Mov_izquierda();
    void Mov_vertical();
    qreal getx() const;
    qreal gety() const;
    qreal getr() const;
    int getCreados() const;
    bool esJugador() const override;
    void Disparar();
    short int getCantMunicion();
    Misil* obtenerDisparo(short int posicion);
    std::vector<Misil*>& getmunicion();
    void actualizarelementos();
    void disparosenemigos(Avion* jugador);
    void setrecargar(bool estado);
    bool getrecargar();
    void setCantmunicion(short int cambio);
    bool getdestruido() const;
    void setVelocidady(qreal newspeed);
    qreal getspeedy() const;
    void setderribados(short int newnumero = 1);
    short int getderribados() const;

    //Colisiones y Físicas
    bool Planes_colision(Avion* jugador);
    bool Balas_colision(Misil* misildisparado);
    bool operator==(Avion* other);
    bool operator!=(Avion* other);
    void recibirImpacto(Proyectil* p) override;

private:
    short int speed;
    qreal speedy;
    short int CantMunicion = 0;
    short int derribados = 0;
    static int creados;
    bool recargar = false;
    qreal r;
    qreal x;
    qreal y;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    bool bando;
    std::vector<Misil*> municion;
    bool destruido = false;
};

#endif // AVION_H
