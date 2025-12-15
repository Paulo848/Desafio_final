#ifndef AGENTE_H
#define AGENTE_H

#include <vector>
#include <qmath.h>
#include "vector2d.h"

class FuerzaArmada;
class Nivel;

// ---- Modos de comportamiento de un grupo ----
enum class ModoGrupo {
    AtaqueDirecto,
    Campamento,
    Rotacion
};

// ---- Estado interno del grupo ----
enum class EstadoGrupo {
    Preparando,
    EnPosicion,
    Atacando,
    Huyendo,
    Muerto
};

// Controlador abstracto de grupos/oleadas
class Agente
{
public:
    explicit Agente(Nivel *nivelPtr);
    virtual ~Agente();

    // ----- Gestión de oleadas -----
    virtual void inicializarRondas(int total) = 0;
    virtual void spawnRonda(int cantidad,
                            qreal radioMin, qreal radioMax,
                            qreal angMinRad, qreal angMaxRad) = 0;

    // ----- Actualización por frame -----
    virtual void actualizar() = 0;

    // ----- Estado de progreso -----
    virtual bool rondaCompletada() const = 0;

    // ----- Acceso al grupo -----
    inline const std::vector<FuerzaArmada*>& getGrupo() const { return grupo; }
    inline int  getRondaActual()     const { return rondaActual; }
    inline int  getTotalRondas()     const { return totalRondas; }

    inline void      setModo(ModoGrupo m)      { modo = m; }
    inline ModoGrupo getModo()          const  { return modo; }

    inline void        setEstado(EstadoGrupo e){ estado = e; }
    inline EstadoGrupo getEstado()       const { return estado; }

    inline void setRondaAsignada(int r)        { rondaAsignada = r; }
    inline int  getRondaAsignada()      const  { return rondaAsignada; }

    inline int getEnemigosRestantes()   const  { return enemigosRestantes; }
    inline int getEnemigosTotales()     const  { return enemigosTotales; }

    inline void setActivo(bool a)              { activo = a; }
    inline bool estaActivo()           const   { return activo; }

protected:
    // Referencias/colecciones
    Nivel *nivel;                        // nivel dueño
    std::vector<FuerzaArmada*> grupo;    // unidades del grupo

    // Progreso interno del agente
    int rondaActual;
    int totalRondas;

    // Configuración de oleadas globales
    int       rondaAsignada;             // ronda global en que aparece
    ModoGrupo modo;                      // modo principal
    EstadoGrupo estado;                  // estado dentro del modo
    bool      activo;                    // si el nivel lo está usando

    // Contadores de enemigos
    int enemigosRestantes;
    int enemigosTotales;
};

#endif // AGENTE_H
