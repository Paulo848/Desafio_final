#include "oleadacadetes.h"

#include "nivel.h"
#include "cadete.h"
#include "obstaculo.h"
#include "constantes_juego.h"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QList>

#include <QRandomGenerator>
#include <QtMath>

#include <limits>

const int OleadaCadetes::COOLDOWN_DISPARO_TICKS = 30;  // ~0.5s si el timer del nivel es 16ms

// =====================================
//  Constructor / Destructor (mínimos)
// =====================================

OleadaCadetes::OleadaCadetes(Nivel *nivelPtr)
    : Agente(nivelPtr),
    ticksDesdeUltDisparo(0)
{
    // Estados por defecto
    accionMoverActiva    = false;
    accionDispararActiva = false;
    focoMovimiento       = Vector2D::nulo();

    radioActivacion      = 0.0;
    llamadoPorOtraOleada = false;
    municionGrupoActual  = 0;
}

//OleadaCadetes::~OleadaCadetes() = default;

void OleadaCadetes::spawnRonda(int cantidad,
                               qreal radioMin, qreal radioMax,
                               qreal angMinRad, qreal angMaxRad)
{
    if (cantidad <= 0)      return;
    if (radioMax <= radioMin)   return;
    if (angMaxRad <= angMinRad) return;
    if (!nivel)                 return;

    enemigosTotales    = cantidad;
    enemigosRestantes  = cantidad;

    // Jugador en (0,0) en coordenadas de escena (según tu diseño)
    Vector2D centro(0.0, 0.0);

    auto *fondo  = nivel->getFondoScroll();
    auto *escena = nivel->getEscena();
    if (!fondo || !escena) return;

    const qreal deltaAng = (angMaxRad - angMinRad) / cantidad;

    for (int i = 0; i < cantidad; ++i) {

        // 1) Ángulo estratificado dentro del rango [angMinRad, angMaxRad]
        qreal angBase = angMinRad + i * deltaAng;
        qreal uAng    = QRandomGenerator::global()->generateDouble(); // [0,1)
        qreal angulo  = angBase + uAng * deltaAng;

        // 2) Radio sesgado hacia radioMax
        qreal uRad = QRandomGenerator::global()->generateDouble();    // [0,1)
        qreal r    = radioMin + (radioMax - radioMin) * qPow(uRad, 1.5);

        // 3) Posición en coordenadas de escena
        Vector2D offset   = Vector2D::desdePolar(r, angulo);
        Vector2D posScene = centro + offset;

        // Convertir a coordenadas locales del fondo
        QPointF posEnFondo = fondo->mapFromScene(posScene.toPointF());

        // 4) Crear cadete enemigo
        Cadete *e = new Cadete(10.0, 0.0, 0.0, false);
        e->setSprite(":/cadete/nivel_3/Cadete_2.png");
        e->setVida(VIDA_ENEMIGO);
        e->setParentItem(fondo);
        e->setPos(posEnFondo);

        // velocidad de enemigos usando constante
        e->setVelocidad(VELOCIDAD_ENEMIGO);

        // Dirección inicial mirando al jugador
        Vector2D dirToPlayer = centro - posScene;
        if (dirToPlayer.magnitud2() > 0.0)
            e->setDireccion(dirToPlayer.normalizado());

        grupo.push_back(e);
        nivel->registrarEnemigo(e);  // el Nivel también los conoce

    }

    // Según el modo, ajustamos estados iniciales
    switch (modo) {
    case ModoGrupo::Campamento:
        estado        = EstadoGrupo::Preparando;
        break;

    case ModoGrupo::Rotacion:
        estado        = EstadoGrupo::Preparando;
        break;

    case ModoGrupo::AtaqueDirecto:
    default:
        estado        = EstadoGrupo::Atacando;
        break;
    }
}

//------------------------------------------------------
//--------------------loop uptadte----------------------
//------------------------------------------------------

void OleadaCadetes::actualizar()
{
    if (!activo)
        return;

    // Actualizar cuántos enemigos siguen vivos
    actualizarEstadoVivosYMuertos();

    if (estado == EstadoGrupo::Muerto || enemigosRestantes == 0) {
        accionMoverActiva    = false;
        accionDispararActiva = false;
        return;
    }

    // Por defecto, las acciones se apagan; el modo las prenderá si toca
    accionMoverActiva    = false;
    accionDispararActiva = false;

    // Traducir el modo en flags/acciones básicas
    switch (modo)
    {
    case ModoGrupo::AtaqueDirecto:
        actualizarModoAtaqueDirecto();
        break;

    case ModoGrupo::Campamento:
        actualizarModoCampamento();
        break;

    case ModoGrupo::Rotacion:
        actualizarModoRotacion();
        break;
    }

    // Aplicar acciones básicas al grupo
    aplicarAccionMover();
    aplicarAccionDisparar();

    actualizarDisparos();
    actualizarMunicionGrupo();
}

void OleadaCadetes::actualizarEstadoVivosYMuertos()
{
    enemigosRestantes = 0;

    for (FuerzaArmada *e : grupo) {
        if (!e) continue;
        if (e->estaMuerto()) continue;
        ++enemigosRestantes;
    }

    if (enemigosRestantes == 0) {
        activo               = false;
        estado               = EstadoGrupo::Muerto;
        accionMoverActiva    = false;
        accionDispararActiva = false;
        llamadoPorOtraOleada = false;
    }
}

void OleadaCadetes::actualizarModoAtaqueDirecto()
{
    accionMoverActiva    = true;
    accionDispararActiva = true;

    if (municionGrupoActual == 0) {
        const int balasPorCadete = 15;

        for (FuerzaArmada *e : grupo) {
            auto *c = dynamic_cast<Cadete*>(e);
            if (!c || c->estaMuerto())
                continue;

            c->definirMunicion(balasPorCadete);
        }
        actualizarMunicionGrupo();
    }

    estado = EstadoGrupo::Atacando;

}

void OleadaCadetes::actualizarModoCampamento()
{
    Vector2D campCenter = calcularCentroCampamento();
    Vector2D centroGrupo = calcularCentroGrupoLocal();
    Vector2D posJugador = calcularPosJugador();

    const qreal radioLlegadaCamp  = 65.0;
    const qreal radioLlegadaCamp2 = radioLlegadaCamp * radioLlegadaCamp;

    const qreal radioAct = (radioActivacion > 0.0 ? radioActivacion : 220.0);
    const qreal radioAct2 = radioAct * radioAct;

    switch (estado)
    {
        case EstadoGrupo::Preparando:
        {
            focoMovimiento       = campCenter;
            accionMoverActiva    = true;
            accionDispararActiva = false;

            Vector2D diffCamp = campCenter - centroGrupo;
            if (diffCamp.magnitud2() <= radioLlegadaCamp2) {
                estado = EstadoGrupo::EnPosicion;
            }
            break;
        }

        case EstadoGrupo::EnPosicion:
        {
            accionMoverActiva    = false;
            accionDispararActiva = false;

            Vector2D diffJugador = posJugador - campCenter;
            if (diffJugador.magnitud2() <= radioAct2) {
                estado = EstadoGrupo::Atacando;
                accionMoverActiva    = true;
                accionDispararActiva = true;
            }
            break;
        }

        case EstadoGrupo::Atacando:
        {
            accionMoverActiva    = true;
            accionDispararActiva = true;

            if (municionGrupoActual == 0) {
                const int balasPorCadete = 15;

                for (FuerzaArmada *e : grupo) {
                    auto *c = dynamic_cast<Cadete*>(e);
                    if (!c || c->estaMuerto())
                        continue;

                    c->definirMunicion(balasPorCadete);
                }
                actualizarMunicionGrupo();
            }
            break;
        }

        default:
        {
            accionMoverActiva    = false;
            accionDispararActiva = false;
            break;
        }
    }
}

void OleadaCadetes::actualizarModoRotacion()
{
    Vector2D centroGrupo = calcularCentroGrupoLocal();
    Vector2D campCenter = calcularCentroCampamento();
    Vector2D posJugador = calcularPosJugador();
    Vector2D diffJugador = posJugador - campCenter;
    Vector2D diffCamp = campCenter - centroGrupo;

    const qreal radioAct = (radioActivacion > 0.0 ? radioActivacion : 220.0);
    const qreal radioAct2 = radioAct * radioAct;

    const qreal radioRetirada    = 25.0;
    const qreal radioRetirada2   = radioRetirada * radioRetirada;

    const qreal radioLlegadaCamp  = 65.0;
    const qreal radioLlegadaCamp2 = radioLlegadaCamp * radioLlegadaCamp;

    if (llamadoPorOtraOleada && estado != EstadoGrupo::Huyendo) {
        llamadoPorOtraOleada = false;

        actualizarMunicionGrupo();
        if (municionGrupoActual == 0) {
            const int balasPorCadete = 15;
            for (FuerzaArmada *e : grupo) {
                auto *c = dynamic_cast<Cadete*>(e);
                if (!c || c->estaMuerto())
                    continue;
                c->definirMunicion(balasPorCadete);
            }
            actualizarMunicionGrupo();
        }
        estado       = EstadoGrupo::Atacando;
        accionMoverActiva    = true;
        accionDispararActiva = true;
    }

    switch (estado)
    {
        case EstadoGrupo::Preparando:
        {
            focoMovimiento       = campCenter;
            accionMoverActiva    = true;
            accionDispararActiva = false;

            if (diffJugador.magnitud2() <= radioAct2) {
                estado      = EstadoGrupo::Atacando;
                accionMoverActiva    = true;
                accionDispararActiva = true;

                const int balasPorCadete = 15;

                for (FuerzaArmada *e : grupo) {
                    auto *c = dynamic_cast<Cadete*>(e);
                    if (!c || c->estaMuerto())
                        continue;
                    c->definirMunicion(balasPorCadete);
                }
                actualizarMunicionGrupo();
                break;
            }

            if (diffCamp.magnitud2() <= radioLlegadaCamp2)  estado = EstadoGrupo::EnPosicion;

            break;
        }

        case EstadoGrupo::EnPosicion:
        {
            accionMoverActiva    = false;
            accionDispararActiva = false;

            if (diffJugador.magnitud2() <= radioAct2) {
                estado = EstadoGrupo::Atacando;
                accionMoverActiva    = true;
                accionDispararActiva = true;
            }
            break;
        }

        case EstadoGrupo::Atacando:
        {
            accionMoverActiva    = true;
            accionDispararActiva = true;

            actualizarMunicionGrupo();

            if (municionGrupoActual == 0 || enemigosRestantes <= enemigosTotales/2) {
                enemigosTotales = 0;
                estado      = EstadoGrupo::Huyendo;
                accionDispararActiva = false;
                accionMoverActiva    = true;
                focoMovimiento       = puntoRetirada;
            }
            break;
        }

        case EstadoGrupo::Huyendo:
        {
            focoMovimiento       = puntoRetirada;
            accionMoverActiva    = true;
            accionDispararActiva = false;

            Vector2D diffRet = puntoRetirada - centroGrupo;
            if (diffRet.magnitud2() <= radioRetirada2) {
                estado       = EstadoGrupo::Preparando;
                accionMoverActiva    = false;
                accionDispararActiva = false;

                for (FuerzaArmada *e : grupo) {
                    auto *c = dynamic_cast<Cadete*>(e);
                    if (!c || c->estaMuerto())
                        continue;
                    c->recargar();
                }
                actualizarMunicionGrupo();
            }
            break;
        }

        default:
        {
            accionMoverActiva    = false;
            accionDispararActiva = false;
            break;
        }
    }
}

void OleadaCadetes::actualizarDisparos()
{
    if (!nivel)
        return;

    // Si este grupo no está en modo de disparar, no hacemos nada
    if (!accionDispararActiva) {
        // Podemos dejar el cooldown corriendo o resetearlo; aquí lo reseteo
        ticksDesdeUltDisparo = 0;
        return;
    }

    // Avanzamos el cooldown del grupo
    ++ticksDesdeUltDisparo;

    // Todavía no toca disparar
    if (ticksDesdeUltDisparo < COOLDOWN_DISPARO_TICKS)
        return;

    // Parámetros de comportamiento
    const int   maxDisparosEsteTick = 3;  // como máximo 3 enemigos disparan a la vez
    int         disparosHechos      = 0;

    for (FuerzaArmada *e : grupo) {
        auto *c = dynamic_cast<Cadete*>(e);
        if (!c || c->estaMuerto())
            continue;

        // Debe estar marcado como "disparando" por la lógica de modo
        if (!c->estaDisparando())
            continue;

        // Sin balas -> nada que hacer
        if (!c->tieneMunicion())
            continue;

        // Un poco de aleatoriedad para que no disparen todos a la vez
        double u = QRandomGenerator::global()->generateDouble();
        if (u > 0.6)               // ~40% probabilidad de que este cadete dispare en esta ráfaga
            continue;

        // Pedimos al nivel que cree la bala
        nivel->disparar(c);
        ++disparosHechos;

        if (disparosHechos >= maxDisparosEsteTick)
            break;
    }

    // Si hubo al menos un disparo, reiniciamos el cooldown
    if (disparosHechos > 0)
        ticksDesdeUltDisparo = 0;
}

bool OleadaCadetes::rondaCompletada() const
{
    return (enemigosRestantes == 0);
}

// ===================================================
//  ACCIÓN BÁSICA: MOVER (ir hacia focoMovimiento)
// ===================================================

void OleadaCadetes::aplicarAccionMover()
{
    if (!accionMoverActiva)
        return;

    for (FuerzaArmada *e : grupo) {

        if (!e || e->estaMuerto())
            continue;

        Vector2D dirBase = Vector2D::nulo();
        if ( estado == EstadoGrupo::Atacando ) dirBase = calcularDireccionHaciaJugador(e);
        else dirBase = calcularDireccionHaciaFoco(e);

        if (dirBase.magnitud2() == 0.0)
            continue;

        // Separación con respecto a otros cadetes
        Vector2D dirSep  = calcularSeparacion(e);

        // Combinar ambas direcciones
        Vector2D dirFinal = combinarDirecciones(dirBase, dirSep);
        if (dirFinal.magnitud2() == 0.0)
            dirFinal = dirBase;

        // Paso según velocidad propia de la unidad
        qreal step = e->getVelocidad();
        if (step <= 0.0) step = 1.0;

        moverUnidadConColisiones(e, dirFinal, step);
    }
}

// ===================================================
//  ACCIÓN BÁSICA: DISPARAR (encender / apagar flag)
// ===================================================

void OleadaCadetes::aplicarAccionDisparar()
{
    for (FuerzaArmada *e : grupo) {
        auto *c = dynamic_cast<Cadete*>(e);
        if (!c || c->estaMuerto())
            continue;

        if (accionDispararActiva) {
            // La lógica fina de munición / cooldown se hará aparte.
            // Aquí solo dejamos marcado que este cadete está en modo “disparando”.
            c->setDisparando(true);
        } else {
            c->setDisparando(false);
        }
    }
}

// ---------------------------------------------------
//  Recalcular “municionGrupoActual”
//  (aquí lo interpretamos como: cuántos cadetes tienen balas)
// ---------------------------------------------------

void OleadaCadetes::actualizarMunicionGrupo()
{
    municionGrupoActual = 0;

    for (FuerzaArmada *e : grupo) {
        auto *c = dynamic_cast<Cadete*>(e);
        if (!c || c->estaMuerto())
            continue;

        if (c->tieneMunicion())
            ++municionGrupoActual;
    }
}

// ===================================================
//  HELPERS DE MOVIMIENTO / STEERING
// ===================================================

// Dirección desde el cadete hacia focoMovimiento
Vector2D OleadaCadetes::calcularDireccionHaciaJugador(FuerzaArmada *e) const
{
    Vector2D pos_jugador = calcularPosJugador();
    Vector2D pos_cadete= Vector2D::nulo();

    if(e != nullptr) pos_cadete = Vector2D(e->pos());
    else return Vector2D::nulo();

    Vector2D toTarget = pos_jugador - pos_cadete;

    if (toTarget.magnitud2() == 0.0)
        return Vector2D::nulo();

    return toTarget.normalizado();
}

Vector2D OleadaCadetes::calcularDireccionHaciaFoco(FuerzaArmada *e) const
{
    // Asumimos que focoMovimiento está en coordenadas locales
    Vector2D pos_ (e->pos());
    Vector2D toTarget = focoMovimiento - pos_;

    if (toTarget.magnitud2() == 0.0)
        return Vector2D::nulo();

    return toTarget.normalizado();
}

Vector2D OleadaCadetes::calcularSeparacion(FuerzaArmada *e) const
{
    const qreal radioSep  = 40.0;
    const qreal radioSep2 = radioSep * radioSep;

    // TODO LOCAL (fondo)
    Vector2D posLocal(e->pos());

    Vector2D dirRef = e->getDireccion();
    if (dirRef.magnitud2() > 0.0) dirRef = dirRef.normalizado();

    Vector2D acumulado = Vector2D::nulo();

    for (FuerzaArmada *otro : grupo) {
        if (!otro || otro == e || otro->estaMuerto()) continue;

        Vector2D posOther(otro->pos());            // <-- LOCAL
        Vector2D diff = posLocal - posOther;
        qreal dist2 = diff.magnitud2();

        if (dist2 < 1e-3) {
            if (dirRef.magnitud2() > 0.0) {
                Vector2D perp(-dirRef.y(), dirRef.x());
                if (perp.magnitud2() > 0.0) acumulado += perp.normalizado() * 5.0;
            }
        } else if (dist2 < radioSep2) {
            acumulado += diff / dist2;
        }
    }
    return acumulado;
}

// Suma base + separación y normaliza
Vector2D OleadaCadetes::combinarDirecciones(const Vector2D &base,
                                            const Vector2D &sep) const
{
    Vector2D out = base + sep;
    if (out.magnitud2() == 0.0)
        return base;
    return out.normalizado();
}

// Mover con colisiones contra Obstaculo / otras FuerzaArmada
void OleadaCadetes::moverUnidadConColisiones(FuerzaArmada *e,
                                             const Vector2D &dir,
                                             qreal step)
{
    if (!e || e->estaMuerto())
        return;

    if (dir.magnitud2() == 0.0 || step <= 0.0)
        return;

    Vector2D dirNorm = dir.normalizado();

    e->setDireccion(dirNorm);

    // Posición local (respecto al parent, que suele ser el fondo)
    QPointF posLocalQ = e->pos();
    Vector2D posLocal(posLocalQ.x(), posLocalQ.y());

    Vector2D newLocal = posLocal + dirNorm * step;
    e->setPos(newLocal.x(), newLocal.y());

    QList<QGraphicsItem*> cols = e->collidingItems();

    for (QGraphicsItem *item : cols) {
        if (auto *obs = dynamic_cast<Obstaculo*>(item)) {
            // Obstáculo físico: usamos reacción específica
            e->setPos(posLocalQ);
            Vector2D react = reaccionarColision(obs, e, step);
            e->setPos(react.x(), react.y());
            return;
        }

        if (auto *fa = dynamic_cast<FuerzaArmada*>(item)) {
            if (fa == e) continue;
            // Otra unidad: retrocedemos y que la separación lo arregle
            e->setPos(posLocalQ);
            return;
        }
    }
}

// ===================================================
//  HELPERS
// ===================================================

// Centro del grupo en coordenadas de escena
Vector2D OleadaCadetes::calcularCentroGrupoLocal() const
{
    Vector2D acumulado = Vector2D::nulo();
    int conteo = 0;
    for (FuerzaArmada *e : grupo) {
        if (!e || e->estaMuerto())
            continue;
        acumulado += Vector2D(e->pos());
        ++conteo;
    }
    if (conteo == 0)
        return Vector2D::nulo();

    return acumulado / static_cast<qreal>(conteo);
}

Vector2D OleadaCadetes::calcularCentroCampamento()
{
    Vector2D campCenter = Vector2D::nulo();
    if (!puntosCampamento.empty()) {
        for (const auto &p : puntosCampamento)
            campCenter += p;
        campCenter /= static_cast<qreal>(puntosCampamento.size());
    } else {
        campCenter = Vector2D(0.0, 0.0);
    }
    return campCenter;
}

Vector2D OleadaCadetes::calcularPosJugador() const {

    Vector2D jugScene = Vector2D::nulo();
    Vector2D playerLocal(nivel->getFondoScroll()->mapFromScene(jugScene.toPointF()));

    return playerLocal;
}

// Reacción al chocar con un obstáculo
Vector2D OleadaCadetes::reaccionarColision(Obstaculo *obs,
                                           FuerzaArmada *cadet,
                                           qreal step)
{
    Vector2D pos_act(cadet->pos());      // LOCAL
    Vector2D dir_act = cadet->getDireccion();
    if (dir_act.magnitud2() == 0.0) return pos_act;
    dir_act = dir_act.normalizado();

    // TODO: obstáculo en LOCAL (porque también es hijo del fondo)
    Vector2D obsLocal(obs->pos());

    // Normal del obstáculo "respecto al jugador", pero TODO en LOCAL
    Vector2D posJugador = calcularPosJugador();
    Vector2D v = (obsLocal - posJugador);
    if (v.magnitud2() == 0.0) v = Vector2D(1,0);
    Vector2D dir_normal_obs = v.normalizado().getNormal();

    qreal dot = dir_act.dot(dir_normal_obs);

    Vector2D new_pos;
    if (dot >= 0)
        new_pos = pos_act + (dir_act - dir_normal_obs).normalizado() * step;
    else
        new_pos = pos_act + (dir_act + dir_normal_obs).normalizado() * step;

    return new_pos;
}

// ===================================================
//  Configuración básica desde Nivel (implementación)
// ===================================================

void OleadaCadetes::setPuntosCampamento(const std::vector<Vector2D> &puntos)
{
    puntosCampamento = puntos;
}

void OleadaCadetes::setPuntoRetirada(const Vector2D &p)
{
    puntoRetirada = p;
}

void OleadaCadetes::setRadioActivacion(qreal r)
{
    radioActivacion = r;
}

bool OleadaCadetes::estaHuyendo() const
{
    return (modo == ModoGrupo::Rotacion &&
            estado == EstadoGrupo::Huyendo &&
            enemigosRestantes > 0);
}

void OleadaCadetes::marcarLlamadoPorOtraOleada()
{
    if (enemigosRestantes <= 0) return;
    if (estado == EstadoGrupo::Huyendo) return;
    llamadoPorOtraOleada = true;
}
