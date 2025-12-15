#ifndef AVION_H
#define AVION_H

#include "fuerzaarmada.h"
#include "misil.h"
#include <QTimer>
#include <QPainter>

class Avion: public FuerzaArmada
{
public:
    Avion(bool _bando = true, qreal posx = 70, qreal posy = 170);
    ~Avion();
    int getCreados() const;
    bool esJugador() const override;
    void Disparar(bool Modo);
    short int getCantMunicion();
    Misil* obtenerDisparo(short int posicion);
    std::vector<Misil*>& getmunicion();
    void actualizarelementos();
    void disparosenemigos(Avion* jugador);
    void setrecargar(bool estado);
    bool getrecargar();
    short int getDanioInfligido() const;
    void setDanioInfligido(short int Danio);
    void setCan_Dispara(bool estado);
    bool Can_Dispara() const;
    void setCantmunicion(short int cambio);
    bool getdestruido() const;
    void setderribados(short int newnumero);
    short int getderribados() const;
    void setCreados(short int valor);
    void setDanhocausado();
    short int getDanhocausado() const;

    //Colisiones y Físicas
    bool Planes_colision(Avion* jugador);
    Vector2D DireccionDisparoParabolico(Avion* Jugador);
    bool Balas_colision(Proyectil* misildisparado);
    bool operator==(Avion* other);
    bool operator!=(Avion* other);
    void recibirImpacto(Proyectil* p) override;
    void Mov_derecha();
    void Mov_izquierda();
    void Mov_up();
    void Mov_down();
    bool AumentarDisparo();
    bool recibiodaño = false;
    void Mov_Combinado();

private:
    short int BalasDisponibles = 20;
    bool Dispara = true;
    short int Danhocausado = 0;
    short int DanioInfligido = 0;
    bool Desplazar = true;
    short int CantMunicion = 0;
    short int derribados = 0;
    static int creados;
    bool recargar = false;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    std::vector<Misil*> municion;
    QPixmap sprite;
};

#endif // AVION_H
