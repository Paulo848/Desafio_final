#include "misil.h"
#include "vector2d.h"
#include "avion.h"

Misil::Misil(FuerzaArmada* duenho, const Vector2D &dir): Proyectil(duenho, dir, duenho -> getVelocidad(), 20, 200){
    double centerX = 0.0, centerY = 0.0;
    if (duenho -> esJugador()){
        sprite.load(":/proyectiles/nivel_1/cohete_2.png");
        centerX = 37.0 / 2.0;
        centerY = 6.0 / 2.0;
    } else {
        velocidad += 3;
        sprite.load(":/proyectiles/nivel_1/cohete_1.png");
        centerX = 42.0 / 2.0;
        centerY = 7.0 / 2.0;
    }
    setTransformOriginPoint(centerX, centerY);
    (duenho -> esJugador()) ? setPos(duenho -> pos().x() + 35, duenho -> pos().y() + 13) : setPos(duenho -> pos().x() - 20, duenho -> pos().y() + 13);
}

QRectF Misil::boundingRect() const{
    return QRectF(0, 0, 42, 7);
}

void Misil::paint(QPainter* painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    painter -> setBrush(Qt::darkRed);
    painter -> drawPixmap(0, 0, sprite);
}

QPainterPath Misil::shape() const {
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void Misil::aplicarImpacto(FuerzaArmada* integrante){
    if (integrante -> estaMuerto()) return; //Verifica que no este muerto.

    if (!integrante -> esJugador() && (esDeJugador() != emisor -> esJugador())){ //Aplica el Impacto solo para enemigos.
        integrante -> recibirImpacto(this);
    }
}

void Misil::aplicarColision(FuerzaArmada* integrante){
    if (!integrante -> estaMuerto()){//Si no esta muerto, aplico logica de Colision entre el Misil y el ente.
        aplicarImpacto(integrante);
    }
    muerto = true;
}

bool Misil::esDeJugador() const{
    return emisor -> esJugador();//Denota si la bala pertenece al jugador.
}

void Misil::Colision_Balas(Misil* other){
    if (!muerto && !other -> muerto){
        if (this -> esDeJugador() != other -> esDeJugador()){
            this -> muerto = true;
            other->muerto = true;
            qDebug() << "Choque de Balas";
        }
    }
}

void Misil::avanzar(){
    if (!muerto){
        Vector2D delta = direccion * velocidad;
        setPos(pos().x() + delta.x(), pos().y() + delta.y());

        if (pos().x() > 1525 + 20 || pos().x() < 22 -20){
            muerto = true;
        }
    }
}

void Misil::Colision_Avion(){
    if (!this -> muerto){
        for (auto *item : collidingItems()) {
            Avion* avion = dynamic_cast<Avion*>(item);
            if (avion) {
                if (avion -> esJugador() != esDeJugador()){
                    qDebug() << avion -> esJugador() << "| Colisiono con Bala.";
                    if (!avion -> esJugador()){
                        avion -> recibirImpacto(this);
                    }
                    this -> muerto = true;
                }
            } else {
                Misil* misil = dynamic_cast<Misil*>(item);
                if (misil){
                    Colision_Balas(misil);
                }
            }

            if (muerto) break;
        }
    }
}

Misil::~Misil(){}


bool Misil::operator==(Misil* other){
    if (pos().x() != other -> pos().x()){
        return false;
    }

    if (pos().y() != other -> pos().y()){
        return false;
    }

    if (velocidad != other -> velocidad){
        return false;
    }

    if (daño != other -> daño){
        return false;
    }

    if (muerto != other -> muerto){
        return false;
    }
    return true;
}

bool Misil::operator!=(Misil* other){
    return !(this == other);
}

/*void Misil::Desplazar(FuerzaArmada* emisor){
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
}*/



/*short int Misil::getdamage() const{
    return damage;
}

qreal Misil::getvelocidad() const{
    return velocidad;
}*/
