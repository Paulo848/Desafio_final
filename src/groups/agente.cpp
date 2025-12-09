#include "agente.h"

Agente::Agente(Nivel *nivelPtr)
    : nivel(nivelPtr),
    rondaActual(1),
    totalRondas(1),
    rondaAsignada(1),
    modo(ModoGrupo::AtaqueDirecto),
    estado(EstadoGrupo::Preparando),
    activo(true),
    enemigosRestantes(0),
    enemigosTotales(0)
{
}

Agente::Agente(short int _RondaActual, short int _TotalRondas): rondaActual(_RondaActual), totalRondas(_TotalRondas){}

Agente::~Agente()
{
    // El Nivel es quien borra las FuerzaArmada del grupo
}
