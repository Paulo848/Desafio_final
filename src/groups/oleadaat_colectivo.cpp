#include "oleadaat_colectivo.h"
#include "avion.h"
#include <QRandomGenerator>

OleadaAt_Colectivo::OleadaAt_Colectivo(){}

OleadaAt_Colectivo::~OleadaAt_Colectivo(){
    for (auto it = grupo.begin(); it != grupo.end();){
        auto* integrante = *it;
        it = grupo.erase(it);
        delete integrante;
    }
}

short int OleadaAt_Colectivo::getCantintegrantes() const{
    return Cantintegrantes;
}

void OleadaAt_Colectivo::Calcular_Desplazamiento(Avion* jugador){
    qreal x, y;
    for (auto it = grupo.begin(); it != grupo.end(); it++){
        Avion* integrante = dynamic_cast<Avion*>(*it);

        x = jugador -> pos().x() - integrante -> pos().x();
        y = jugador -> pos().y() - integrante -> pos().y();
        integrante -> setDireccion(Vector2D (x, y).normalizado());
    }
}

void OleadaAt_Colectivo::setRondas(){
    rondaActual++;
    Cantintegrantes++;
    OleadaActual = 0;
    CantOleadas += 2;
}

void OleadaAt_Colectivo::Generarenemigos(){
    short int cont = 0; short int posy; bool crear;
    while (cont < Cantintegrantes){
        crear = true;
        posy = QRandomGenerator::global()->bounded(0, 326);
        if (cont != 0){
            for (auto it = grupo.begin(); it != grupo.end(); it++){
                Avion* enemyIA = dynamic_cast<Avion*>(*it);
                enemyIA -> setCan_Dispara(false);
                if (std::abs(posy - enemyIA -> pos().y()) < 35){
                    crear = false;
                } else if (std::abs(posy - enemyIA -> pos().y()) < 35){
                    crear = false;
                }
            }
        }
        if (crear){
            if (OleadaActual == 0){
                grupo.push_back(new Avion(false, 1495, posy));
                grupo[cont] -> setVelocidad(20);
                grupo[cont] -> setVida(5000);
                Avion* enemyIA = dynamic_cast<Avion*>(grupo[cont]);
                enemyIA -> setCreados(-1);
            } else {
                grupo[cont] -> setPos(1495, posy);
                grupo[cont] -> muerto = false;
            }
            cont++;
        }
    }
    OleadaActual++;
}

bool OleadaAt_Colectivo::Generarnewround(){
    return rondaActual <= totalRondas;
}

bool OleadaAt_Colectivo::rondaCompletada() const {
    return OleadaActual == CantOleadas;
}

short int OleadaAt_Colectivo::getCantOleadas() const{
    return CantOleadas;
}

bool OleadaAt_Colectivo::GenerarnuevaOleada(){
    if (!rondaCompletada()){
        for (auto integrante : grupo){
            if (!integrante -> muerto){
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

short int OleadaAt_Colectivo::getOleadaActual() const{
    return OleadaActual;
}
void OleadaAt_Colectivo::inicializarRondas(int Total){
    totalRondas = Total;
}

void OleadaAt_Colectivo::actualizar(){
    for (auto integrante : grupo){
        integrante -> setPos(integrante -> pos().x(), integrante -> pos().y());
    }
}

void OleadaAt_Colectivo::spawnRonda(int cantidad, qreal radioMin, qreal radioMax, qreal angMinRad, qreal angMaxRad){
    return;
}
