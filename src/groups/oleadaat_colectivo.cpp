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
    for (auto it = grupo.begin(); it != grupo.end(); it++){
        Avion* integrante = dynamic_cast<Avion*>(*it);

        qreal dx = jugador->getx() - integrante->getx();
        qreal dy = jugador->gety() - integrante->gety();

        qreal magnitud = sqrt(dx*dx + dy*dy);

        // Evitar división por cero
        if (magnitud < 0.001) magnitud = 0.001;

        qreal vy = (dy / magnitud);
        integrante->setVelocidady(vy * integrante -> getspeedy());
    }
}

void OleadaAt_Colectivo::setRondas(){
    rondaActual++;
    Cantintegrantes++;
    OleadaActual = 1;
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
                if (std::abs(posy - enemyIA -> gety()) < 35){
                    crear = false;
                } else if (std::abs(posy - enemyIA -> gety()) < 35){
                    crear = false;
                }
            }
        }
        if (crear){
            grupo.push_back(new Avion(35, 1500, posy, false, 12));
            cont++;
        }
    }
    OleadaActual++;
}

bool OleadaAt_Colectivo::Generarnewround(){
    if (rondaActual <= totalRondas){
        bool derribados = true;
        for (auto it = grupo.begin(); it != grupo.end(); it++){
            Avion* enemyIA = dynamic_cast<Avion*>(*it);
            if (!enemyIA -> getdestruido()){
                derribados = false;
            }
        }
        return derribados;
    } else {
        return false;
    }
}

bool OleadaAt_Colectivo::rondaCompletada() const{
    return OleadaActual > CantOleadas;
}

void OleadaAt_Colectivo::inicializarRondas(int Total){
    totalRondas = Total;
}

void OleadaAt_Colectivo::actualizar(){
    return;
}

void OleadaAt_Colectivo::spawnRonda(int cantidad, qreal radioMin, qreal radioMax, qreal angMinRad, qreal angMaxRad){
    return;
}
