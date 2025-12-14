#include "avion.h"

Avion::Avion(bool _bando, qreal posx, qreal posy): FuerzaArmada(35, _bando, (_bando) ? 2000 : 400){
    direccion = Vector2D(1.0, 0.0);
    if (_bando){
        sprite.load(":/entes/nivel_1/Avio_aliado.png");
        velocidad = 5;
    } else {
        sprite.load(":/entes/nivel_1/Avio_enemigo.png");
        velocidad = 3;
    }

    double centerX = 76.0 / 2.0; // 38.0
    double centerY = 24.0 / 2.0; // 12.0
    setTransformOriginPoint(centerX, centerY);
    creados++;
    setPos(posx, posy);
}

int Avion::creados = 0;

void Avion::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    painter -> drawPixmap(0, 0, sprite);
}

QRectF Avion::boundingRect() const{
    return QRectF(0, 0, 76, 24);
}

QPainterPath Avion::shape() const {
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}

void Avion::Mov_derecha(){
    direccion.set(1.0, 0.0);
    Vector2D direcc = direccion * velocidad;
    if (jugador){
        if (pos().x() + direcc.x() > 1525){
            direcc.setX(0);
        }
    } else if (pos().x() + direcc.x() > 1525 + radio){
        muerto = true;
    }
    setPos(pos().x() + direcc.x(), pos().y());
}

void Avion::Mov_izquierda(){
    direccion.set(1.0, 0.0);
    Vector2D direcc = direccion * (-velocidad);
    if (jugador){
        if (pos().x() + direcc.x() < 22){
            direcc.setX(0);
        }
    } else if (pos().x() + direcc.x() < 22 - radio){
        muerto = true;
    }
    setPos(pos().x() + direcc.x(), pos().y());
}

void Avion::Mov_up(){
    direccion.set(0.0, 1.0);
    Vector2D direcc = direccion * (-velocidad);
    if (jugador){
        if (pos().y() + direcc.y() < 0){
            direcc.setY(0);
        }
    } else if (pos().y() + direcc.y() < 0 - radio){
        muerto = true;
    }
    setPos(pos().x(), pos().y() + direcc.y());
}

void Avion::Mov_down(){
    direccion.set(0.0, 1.0);
    Vector2D direcc = direccion * velocidad;
    if (jugador){
        if (pos().y() + direcc.y() > 325){
            direcc.setY(0);
        }
    } else if (pos().y() + direcc.y() > 325 + radio){
        muerto = true;
    }
    setPos(pos().x(), pos().y() + direcc.y());
}

void Avion::Mov_Combinado(){
    Vector2D direcc = direccion * velocidad;
    setPos(pos().x() + direcc.x(), pos().y() + direcc.y());

    if (pos().x() < 22 - radio || pos().y() < 0 - radio || pos().y() > 325 + radio){
        muerto = true;
    }
}

int Avion::getCreados() const{
    return creados;
}

bool Avion::esJugador() const{
    return jugador;
}

void Avion::Disparar(bool Modo){
    if (!Modo){
        if (jugador){
            Vector2D direccionbala(1.0, 0.0);
            municion.push_back(new Misil(this, direccionbala));
        } else {
            Vector2D direccionbala(-1.0, 0.0);
            municion.push_back(new Misil(this, direccionbala));
        }
    } else {
        Vector2D direccionbala(1.0, 1.0);
        municion.push_back(new Misil(this, direccionbala.normalizado()));
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

bool Avion::Planes_colision(Avion* entidad){
    if (this->collidesWithItem(entidad) && (esJugador() != entidad -> esJugador())){
        if (!jugador){
            vida -= 400;
            entidad -> setVida(entidad -> getVida() - 500);
            entidad -> setDanioInfligido(400);
            if (entidad -> getVida() <= 0){
                entidad -> setVida(0);
                entidad -> muerto = true;
            }
        }

        if (vida <= 0){
            vida = 0;
            muerto = true;
            if (Dispara){
                entidad -> setderribados(entidad -> getderribados() + 1);
            }
        }
        return true;
    } else {
        return false;
    }
}

short int Avion::getDanioInfligido() const{
    return DanioInfligido;
}

void Avion::setDanioInfligido(short int Danio){
    DanioInfligido += Danio;
}

Misil* Avion::obtenerDisparo(short int posicion){
    return municion[posicion];
}

std::vector<Misil*>& Avion::getmunicion(){
    return municion;
}

void Avion::setderribados(short int newnumero){
    derribados = newnumero;
}

short int Avion::getderribados() const{
    return derribados;
}

void Avion::recibirImpacto(Proyectil* p){
    vida -= p -> getDaño();
    if (vida <= 0){
        vida = 0;
        muerto = true;
        if (p -> esDeJugador()){
            Avion* jugador = dynamic_cast<Avion*>(p -> getemisor());
            jugador -> setderribados(jugador -> getderribados() + 1);
        }
    }
}

void Avion::actualizarelementos(){
    for (auto disparadas = municion.begin(); disparadas != municion.end();){
        Misil* misiles = *disparadas;
        if (misiles -> muerto){
            CantMunicion--;
            disparadas = municion.erase(disparadas);
            delete misiles;
        } else {
            disparadas++;
        }
    }
}

Avion::~Avion(){
    for (auto it = municion.begin(); it != municion.end();){
        Misil* misil = *it;
        it = municion.erase(it);
        delete misil;
    }
}

void Avion::setrecargar(bool estado){
    recargar = estado;
}

bool Avion::getrecargar(){
    return recargar;
}

bool Avion::Can_Dispara() const{
    return Dispara;
}

void Avion::setCan_Dispara(bool estado){
    Dispara = estado;
}

void Avion::setCreados(short int valor){
    creados += valor;
}
bool Avion::operator==(Avion* other){
    if (pos().x() != other -> pos().x()){
        return false;
    }

    if (pos().y() != other -> pos().y()){
        return false;
    }

    if (CantMunicion != other -> getCantMunicion()){
        return false;
    }

    if (jugador != other -> esJugador()){
        return false;
    }

    if (velocidad != other -> getVelocidad()){
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
