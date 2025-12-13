#ifndef OLEADAAT_COLECTIVO_H
#define OLEADAAT_COLECTIVO_H

#include "agente.h"
#include "avion.h"

class Nivel_1;
class OleadaAt_Colectivo: public Agente
{
public:
    OleadaAt_Colectivo();
    ~OleadaAt_Colectivo();
    short int getCantintegrantes() const;
    short int getOleadaActual() const;
    short int getCantOleadas() const;
    void Calcular_Desplazamiento(Avion* jugador);
    void setRondas();
    void Generarenemigos();
    bool Generarnewround();
    void inicializarRondas(int total) override;
    void spawnRonda(int cantidad,
                            qreal radioMin, qreal radioMax,
                            qreal angMinRad, qreal angMaxRad) override;

    // ----- Actualización por frame -----
    void actualizar() override;

    // ----- Estado de progreso -----
    bool rondaCompletada() const override;
    bool GenerarnuevaOleada();

private:
    short int Cantintegrantes = 2;
    short int CantOleadas = 3;
    short int OleadaActual = 0;
};

#endif // OLEADAAT_COLECTIVO_H
