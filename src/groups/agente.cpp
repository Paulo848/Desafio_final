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

Agente::~Agente()
{
    // El Nivel es quien borra las FuerzaArmada del grupo
}
