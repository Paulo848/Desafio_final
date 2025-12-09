#include "misil.h"

Misil::Misil(qreal _x, qreal _y): x(_x), y(_y), Proyectil(){
    setPos(x,y);
}

QRectF Misil::boundingRect() const{
    return QRectF(0, 0, 20, 7);
}

void Misil::paint(QPainter* painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    painter -> setBrush(Qt::darkRed);
    painter -> drawRect(boundingRect());
}

Misil::~Misil(){}

void Misil::Desplazar(FuerzaArmada* emisor){
    //qDebug() << "se trato de desplazar";
    if (!crashed){
        if (emisor -> esJugador()){
            x += ((emisor -> getVelocidad())*.45 + (velocidad)/10);
        } else{
            x -= ((emisor -> getVelocidad())*.7 + (10+velocidad)/10);
        }
        if (emisor -> esJugador()){
            if (x > 1600){
                crashed = true;
            }
        } else if (x < 0){
            crashed = true;
        }
        setPos(x, y);
        //qDebug() << "se desplazo";
    }
}

qreal Misil::getx() const{
    return x;
}

qreal Misil::gety() const{
    return y;
}

bool Misil::getcrashed() const{
    return crashed;
}

void Misil::setcrashed(bool estado){
    crashed = estado;
}

bool Misil::Colision_Balas(Misil* other){
    if (this -> collidesWithItem(other)){
        crashed = true;
        other -> setcrashed(true);
        qDebug() << "colision";
        return true;
    } else {
        return false;
    }
}

short int Misil::getdamage() const{
    return damage;
}

qreal Misil::getvelocidad() const{
    return velocidad;
}

bool Misil::operator==(Misil* other){
    if (x != other -> getx()){
        return false;
    }
    if (y != other -> gety()){
        return false;
    }
    if (velocidad != other -> getvelocidad()){
        return false;
    }
    if (damage != other -> getdamage()){
        return false;
    }
    if (crashed != other -> getcrashed()){
        return false;
    }
    return true;
}

bool Misil::operator!=(Misil* other){
    return !(this == other);
}

void Misil::aplicarColision(FuerzaArmada* integrante){
    return;
}

bool Misil::esDeJugador() const{
    return true;
}

void Misil::aplicarImpacto(FuerzaArmada* integrante){
    return;
}

void Misil::avanzar(){
    return;
}
