#include "avion.h"

Avion::Avion(qreal _r, qreal _x, qreal _y, bool _bando, short int _speed): r(_r), x(_x), y(_y), speed(_speed), bando(_bando), speedy(_speed){
    setPos(x,y);
    vida = 2000;
    if (!bando){
        vida = 400;
        creados++;
    }
}

int Avion::creados = 0;

void Avion::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    painter -> setBrush(Qt::darkGreen);
    painter -> drawEllipse(boundingRect());
}

QRectF Avion::boundingRect() const{
    return QRectF(0, 0, r, r);
}

void Avion::Mov_derecha(){
    if (bando){
        x += speed + speed/2;
        if (x > 1525){
            x = 1525;
        }
    } else{
        x += speed;
    }
    setPos(x, y);
}

void Avion::Mov_izquierda(){
    if (bando){
        x -= speed + speed/2;
        if (x < 22){
            x = 22;
        }
    } else{
        x -= speed;
        if (x < 22-35){
            destruido = true;
        }
    }
    setPos(x, y);
}

void Avion::Mov_vertical(){
    if (bando){
        y += speedy + speedy/2;
        if (y < 0){
            y = 0;
        } else if (y > 325){
            y = 325;
        }
    } else {
        y += speedy - speedy/2;
    }
    setPos(x, y);
}

qreal Avion::getx() const{
    return x;
}

qreal Avion::gety() const{
    return y;
}

int Avion::getCreados() const{
    return creados;
}


bool Avion::Planes_colision(Avion* jugador){
    if (this->collidesWithItem(jugador) && this -> esJugador() != jugador -> esJugador()){
        if (!bando){
            vida = 0;
            destruido = true;
        } else {
            /*if (vida - 500 < 0){
                vida = 0;
            } else {
                vida -= 500;
            }*/
        }
        return true;
    } else {
        return false;
    }
}

bool Avion::esJugador() const{
    return bando;
}

void Avion::Disparar(){
    if (!recargar){
        if (bando){
            municion.push_back(new Misil(x+r, y+13));
        } else {
            municion.push_back(new Misil(x-20, y+13));
        }
    }
}

void Avion::setCantmunicion(short int cambio){
    CantMunicion += cambio;
    /*if (cambio > 0){
        if (CantMunicion == 4){
            qDebug() << CantMunicion;
            recargar = true;
        }
    }*/
}

short int Avion::getCantMunicion(){
    return CantMunicion;
}

qreal Avion::getr() const {
    return r;
}

Misil* Avion::obtenerDisparo(short int posicion){
    return municion[posicion];
}

std::vector<Misil*>& Avion::getmunicion(){
    return municion;
}

bool Avion::Balas_colision(Misil* misildisparado){
    if (this -> collidesWithItem(misildisparado) && !(misildisparado -> getcrashed())){
        if (misildisparado -> getdamage() > vida){
            vida = 0;
            destruido = true;
        } else {
            vida -= misildisparado -> getdamage();
        }
        misildisparado -> setcrashed(true);
        return true;
    } else {
        return false;
    }
}

Avion::~Avion(){
    for (auto it = municion.begin(); it != municion.end();){
        Misil* misil = *it;
        it = municion.erase(it);
        delete misil;
    }
}

void Avion::actualizarelementos(){
    for (auto disparadas = municion.begin(); disparadas != municion.end();){
        Misil* misiles = *disparadas;
        if (misiles -> getcrashed()){
            CantMunicion--;
            disparadas = municion.erase(disparadas);
            delete misiles;
        } else {
            disparadas++;
        }
    }
    /*if (municion.size() == 0){
        QTimer::singleShot(4000, [this](){
            this -> setrecargar(false);
        });
    }*/
}

bool Avion::operator==(Avion* other){
    bool igualdad = true;
    if (x != other -> getx()){
        return false;
    }
    if (y != other -> gety()){
        return false;
    }
    if (CantMunicion != other -> getCantMunicion()){
        return false;
    }
    if (bando != other -> esJugador()){
        return false;
    }
    if (speed != other -> getVelocidad()){
        return false;
    }
    if (r != other -> getr()){
        return false;
    }
    if (municion.size() != other -> getmunicion().size()){
        return false;
    } else {
        for (size_t i = 0; i < municion.size(); i++){
            if (municion[i] != (other -> getmunicion()[i])){
                return false;
            }
        }
    }
    return true;
}

bool Avion::operator!=(Avion* other){
    return !(this == other);
}

void Avion::setrecargar(bool estado){
    recargar = estado;
}

bool Avion::getrecargar(){
    return recargar;
}

void Avion::setVelocidady(qreal newspeed){
    speedy = newspeed;
}

qreal Avion::getspeedy() const{
    return speedy;
}

bool Avion::getdestruido() const{
    return destruido;
}

void Avion::setderribados(short int newnumero){
    derribados += newnumero;
}

short int Avion::getderribados() const{
    return derribados;
}

void Avion::recibirImpacto(Proyectil* p){
    if (vida - p -> getDaño() < 0){
        vida = 0;
    } else {
        vida -= p -> getDaño();
    }
}
