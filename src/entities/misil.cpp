#include "misil.h"
#include "vector2d.h"
#include "avion.h"
#define GRAVEDAD 4.5f

Misil::Misil(FuerzaArmada* duenho, const Vector2D &dir): Proyectil(duenho, dir, duenho -> getVelocidad(), 20, 200){
    double centerX = 0.0, centerY = 0.0;
    if (duenho -> esJugador()){
        sprite.load(":/proyectiles/nivel_1/cohete_2.png");
    } else {
        velocidad += 3;
        sprite.load(":/proyectiles/nivel_1/cohete_1.png");

    }
    ancho = sprite.width();
    largo = sprite.height();
    centerX = ancho / 2.0;
    centerY = largo / 2.0;
    setTransformOriginPoint(centerX, centerY);
    (duenho -> esJugador()) ? setPos(duenho -> pos().x() + 35, duenho -> pos().y() + 13) : setPos(duenho -> pos().x() - 20, duenho -> pos().y() + 13);
}

void Misil::setDireccionCaida(){
    setRotation(45);
}

QRectF Misil::boundingRect() const{
    return QRectF(0, 0, ancho, largo);
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
    if (!other -> muerto){
        if (this -> esDeJugador() != other -> esDeJugador()){
            if (this -> collidesWithItem(other)){
                this -> muerto = true;
                dañoAplicado = true;
                other -> muerto = true;
                other -> setdañoAplicado(true);
                qDebug() << "Choque de Balas";
            }
        }
    }
}

void Misil::setdañoAplicado(bool estado){
    dañoAplicado = estado;
}

void Misil::avanzar(){
    if (!muerto){
        Vector2D delta = direccion * velocidad;
        setPos(pos().x() + delta.x(), pos().y() + delta.y());

        if (direccion.y() > 0){
            setDireccionCaida();
            direccion.setY(direccion.y() + GRAVEDAD);
            direccion.setX(direccion.x() + velocidad/3);
            direccion = direccion.normalizado();
        }
        if (pos().x() > 1525 + ancho || pos().x() < 22 - ancho || pos().y() > 360 + largo){
            muerto = true;
        }
    }
}

void Misil::Colision_Avion(){
    for (auto *item : collidingItems()) {
        if (muerto || dañoAplicado)break;
        Avion* avion = dynamic_cast<Avion*>(item);
        if (avion) {
            if (avion -> esJugador() != esDeJugador()){
                qDebug() << avion -> esJugador() << "| Colisiono con Bala.";
                if (!avion -> esJugador()){
                    avion -> recibirImpacto(this);
                }
                if (this -> esDeJugador()){
                    Avion* jugador = dynamic_cast<Avion*>(getemisor());
                    jugador -> setDanioInfligido(getDaño());
                } else {
                    Avion* enemigo = dynamic_cast<Avion*>(getemisor());
                    enemigo -> setDanioInfligido(getDaño());
                }
                this -> muerto = true;
                this -> dañoAplicado = true;
                break;
            }
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
