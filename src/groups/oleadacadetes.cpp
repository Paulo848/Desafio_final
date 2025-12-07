#include "oleadacadetes.h"
#include "nivel.h"
#include "cadete.h"
#include "obstaculo.h"

#include <QRandomGenerator>
#include <QtMath>
#include <QGraphicsItem>
#include <QList>
#include <limits>

// =====================================
//  Constructor / Destructor (mínimos)
// =====================================

OleadaCadetes::OleadaCadetes(Nivel *nivelPtr)
    : Agente(nivelPtr)
{
    // Estados por defecto
    accionMoverActiva    = false;
    accionDispararActiva = false;
    focoMovimiento       = Vector2D::nulo();

    radioActivacion      = 0.0;
    estadoRotacion       = EstadoRotacion::EnCampamento;
    pidiendoRefuerzo     = false;
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
        Cadete *e = new Cadete(10.0, 0.0, 0.0, /*esJugador=*/false);
        e->setParentItem(fondo);
        e->setPos(posEnFondo);

        // Dirección inicial mirando al jugador
        Vector2D dirToPlayer = centro - posScene;
        if (dirToPlayer.magnitud2() > 0.0)
            e->setDireccion(dirToPlayer.normalizado());

        escena->addItem(e);

        grupo.push_back(e);
        nivel->registrarEnemigo(e);  // el Nivel también los conoce
    }

    // Según el modo, ajustamos estados iniciales
    switch (modo) {
    case ModoGrupo::Campamento:
        estado        = EstadoGrupo::Preparando;
        estadoRotacion = EstadoRotacion::EnCampamento; // aunque no se use si no es Rotacion
        break;

    case ModoGrupo::Rotacion:
        estado        = EstadoGrupo::Preparando;
        estadoRotacion = EstadoRotacion::EnCampamento;
        break;

    case ModoGrupo::AtaqueDirecto:
    default:
        estado        = EstadoGrupo::Atacando;
        break;
    }
}

void OleadaCadetes::inicializarRondas(int total)
{
    // Número de "sub-rondas" internas que podría manejar este agente.
    totalRondas = (total > 0) ? total : 1;
    rondaActual = 1;

    // El agente arranca activo; aún no sabemos cuántos enemigos tiene
    // hasta que hagamos spawnRonda.
    activo             = true;
    enemigosTotales    = 0;
    enemigosRestantes  = 0;

    // Estado base según el modo actual
    switch (modo) {
    case ModoGrupo::Campamento:
    case ModoGrupo::Rotacion:
        estado = EstadoGrupo::Preparando;
        break;
    case ModoGrupo::AtaqueDirecto:
    default:
        estado = EstadoGrupo::Atacando;
        break;
    }

    // Estado interno específico para Rotación
    if (modo == ModoGrupo::Rotacion) {
        estadoRotacion = EstadoRotacion::EnCampamento;
    }

    // Reset de flags de acciones y coordinación
    accionMoverActiva    = false;
    accionDispararActiva = false;
    pidiendoRefuerzo     = false;
    llamadoPorOtraOleada = false;
    municionGrupoActual  = 0;
}

//_______________________________________________________
//--------------------------loop uptadte ------------------
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

    // Modos anteriores que aún puedas tener, por ahora se comportan
    // como ataque directo para no dejar grupos "muertos":
    case ModoGrupo::Flanqueo:
    case ModoGrupo::Emboscada:
        actualizarModoAtaqueDirecto();
        break;
    }

    // Aplicar acciones básicas al grupo
    if (accionMoverActiva)
        aplicarAccionMover();

    aplicarAccionDisparar();

    // Mantener municionGrupoActual actualizado para decisiones futuras
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

        // En modo Rotación, también podemos marcar su estado interno como
        // “fuera de combate”.
        estadoRotacion       = EstadoRotacion::Huyendo;
        pidiendoRefuerzo     = false;
        llamadoPorOtraOleada = false;
    }
}

void OleadaCadetes::actualizarModoAtaqueDirecto()
{
    // Jugador asumido en (0,0) en coordenadas de escena
    focoMovimiento       = Vector2D(0.0, 0.0);

    // Siempre perseguimos al jugador
    accionMoverActiva    = true;

    // Queremos que el grupo esté en modo "disparando"
    accionDispararActiva = true;

    // Actualizamos cuántos cadetes tienen balas
    actualizarMunicionGrupo();

    // Si absolutamente nadie tiene munición, asignamos cargadores
    if (municionGrupoActual == 0) {
        const int balasPorCadete = 30;  // ajustable

        for (FuerzaArmada *e : grupo) {
            auto *c = dynamic_cast<Cadete*>(e);
            if (!c || c->estaMuerto())
                continue;

            c->definirMunicion(balasPorCadete);
        }

        // Recalcular después de asignar
        actualizarMunicionGrupo();
    }
}

void OleadaCadetes::actualizarModoCampamento()
{
    // Punto de campamento: usamos el promedio de los puntos dados.
    // Si no hay puntos definidos, tomamos (0,0) como fallback.
    Vector2D campCenter = Vector2D::nulo();
    if (!puntosCampamento.empty()) {
        for (const auto &p : puntosCampamento)
            campCenter += p;
        campCenter /= static_cast<qreal>(puntosCampamento.size());
    } else {
        campCenter = Vector2D(0.0, 0.0);
    }

    Vector2D centroGrupo = calcularCentroGrupoScene();
    Vector2D posJugador(0.0, 0.0);

    const qreal radioLlegadaCamp  = 65.0;
    const qreal radioLlegadaCamp2 = radioLlegadaCamp * radioLlegadaCamp;

    const qreal radioAct = (radioActivacion > 0.0 ? radioActivacion : 220.0);
    const qreal radioAct2 = radioAct * radioAct;

    switch (estado)
    {
    case EstadoGrupo::Preparando:
    {
        // Fase: ir al punto de campamento
        focoMovimiento       = campCenter;
        accionMoverActiva    = true;
        accionDispararActiva = false;

        // Si el centro del grupo ya está suficientemente cerca del campamento,
        // consideramos que están "en posición".
        Vector2D diffCamp = campCenter - centroGrupo;
        if (diffCamp.magnitud2() <= radioLlegadaCamp2) {
            estado = EstadoGrupo::EnPosicion;
        }
        break;
    }

    case EstadoGrupo::EnPosicion:
    {
        /*
         *
        qDebug() << "        magnitud : | " << diffJugador.magnitud();
        qDebug() << "dif = |" << diffJugador.x() << " , " << diffJugador.y();
        qDebug() << " campoCenterScene " << campCenterSce.x() << " , " << campCenterSce.y();
        qDebug() << " campoCenter " << campCenterSce.x() << " , " << campCenterSce.y();
        qDebug() << " jugador "  << posJugador.x() << " , " << posJugador.y();
        */
        // Fase: quietos en campamento, sin disparar.
        accionMoverActiva    = false;
        accionDispararActiva = false;

        // Si el jugador se acerca lo suficiente al campamento,
        // activamos un ataque directo (y dejamos que el ciclo
        // continúe como AtaqueDirecto de ahora en adelante).
        Vector2D campCenterSce(nivel->getFondoScroll()->mapToScene(campCenter.toPointF()));
        Vector2D diffJugador = posJugador - campCenterSce;
        if (diffJugador.magnitud2() <= radioAct2) {
            // Cambiamos el modo global del agente a AtaqueDirecto
            // y dejamos que en el siguiente tick se use
            // actualizarModoAtaqueDirecto().
            modo   = ModoGrupo::AtaqueDirecto;
            estado = EstadoGrupo::Atacando;

            // Opcionalmente, preconfiguramos ya el foco y flags:
            focoMovimiento       = posJugador;
            accionMoverActiva    = true;
            accionDispararActiva = true;
        }
        break;
    }

    case EstadoGrupo::Atacando:
        // Una vez que el campamento "salta" a ataque, simplemente
        // delegamos en el comportamiento de ataque directo.
        // (El switch principal de actualizar() será el que llame
        // a actualizarModoAtaqueDirecto() en los frames siguientes.)
        focoMovimiento       = posJugador;
        accionMoverActiva    = true;
        accionDispararActiva = true;
        break;

    case EstadoGrupo::Muerto:
    default:
        // Sin acciones: grupo ya no opera.
        accionMoverActiva    = false;
        accionDispararActiva = false;
        break;
    }
}

void OleadaCadetes::actualizarModoRotacion()
{
    Vector2D posJugador(0.0, 0.0);
    Vector2D centroGrupo = calcularCentroGrupoScene();



    // Centro de campamento para este grupo
    Vector2D campCenter = Vector2D::nulo();
    if (!puntosCampamento.empty()) {
        for (const auto &p : puntosCampamento)
            campCenter += p;
        campCenter /= static_cast<qreal>(puntosCampamento.size());
    } else {
        campCenter = Vector2D(0.0, 0.0);
    }
    Vector2D campCenterSce(nivel->getFondoScroll()->mapToScene(campCenter.toPointF()));

    const qreal radioAct = (radioActivacion > 0.0 ? radioActivacion : 220.0);
    const qreal radioAct2 = radioAct * radioAct;

    const qreal radioRetirada    = 25.0;
    const qreal radioRetirada2   = radioRetirada * radioRetirada;

    if (llamadoPorOtraOleada && estadoRotacion != EstadoRotacion::Huyendo) {
        llamadoPorOtraOleada = false;
        pidiendoRefuerzo     = false;

        // Asegurar que haya munición
        actualizarMunicionGrupo();
        if (municionGrupoActual == 0) {
            const int balasPorCadete = 20;   // mismo número que usas en Rotación
            for (FuerzaArmada *e : grupo) {
                auto *c = dynamic_cast<Cadete*>(e);
                if (!c || c->estaMuerto())
                    continue;
                c->definirMunicion(balasPorCadete);
            }
            actualizarMunicionGrupo();
        }

        estadoRotacion       = EstadoRotacion::Atacando;
        focoMovimiento       = posJugador;
        accionMoverActiva    = true;
        accionDispararActiva = true;
    }

    switch (estadoRotacion)
    {
    case EstadoRotacion::EnCampamento:
    {
        // Igual que un campamento normal:
        // ir al campCenter y esperar al jugador.
        focoMovimiento       = campCenter;
        accionMoverActiva    = true;
        accionDispararActiva = false;

        // ¿El jugador se acercó al campamento?
        Vector2D diffJugador = posJugador - campCenterSce;
        if (diffJugador.magnitud2() <= radioAct2) {
            // Pasar a fase de ataque dentro del modo Rotación.
            estadoRotacion      = EstadoRotacion::Atacando;
            accionMoverActiva    = true;
            accionDispararActiva = true;
            focoMovimiento       = posJugador;

            // Asignar munición inicial a cada cadete del grupo
            const int balasPorCadete = 20;  // puedes ajustar distinto a AtaqueDirecto

            for (FuerzaArmada *e : grupo) {
                auto *c = dynamic_cast<Cadete*>(e);
                if (!c || c->estaMuerto())
                    continue;

                c->definirMunicion(balasPorCadete);
            }

            // Recalcular la munición agregada del grupo
            actualizarMunicionGrupo();
        }
        break;
    }

    case EstadoRotacion::Atacando:
    {
        // Fase de presión: ir hacia el jugador + disparar.
        focoMovimiento       = posJugador;
        accionMoverActiva    = true;
        accionDispararActiva = true;

        // Actualizar munición agregada para decidir retirada
        actualizarMunicionGrupo();

        // Criterios de retirada: sin munición o grupo muy reducido.
        if (municionGrupoActual == 0 || enemigosRestantes <= enemigosTotales/2) {
            enemigosTotales = 0;
            estadoRotacion      = EstadoRotacion::Huyendo;
            accionDispararActiva = false;
            accionMoverActiva    = true;
            focoMovimiento       = puntoRetirada;

            // Levantamos la petición de relevo para que Nivel lo vea.
            pidiendoRefuerzo     = true;
        }
        break;
    }

    case EstadoRotacion::Huyendo:
    {
        // Fase de huida hacia puntoRetirada.
        focoMovimiento       = puntoRetirada;
        accionMoverActiva    = true;
        accionDispararActiva = false;

        // ¿Ya llegamos lo bastante cerca del punto de retirada?
        Vector2D diffRet = puntoRetirada - centroGrupo;
        if (diffRet.magnitud2() <= radioRetirada2) {
            // Entramos en fase de "esperando orden" (recargando).
            estadoRotacion       = EstadoRotacion::EnCampamento;
            accionMoverActiva    = false;
            accionDispararActiva = false;

            // Recargar a todos los cadetes vivos
            for (FuerzaArmada *e : grupo) {
                auto *c = dynamic_cast<Cadete*>(e);
                if (!c || c->estaMuerto())
                    continue;
                c->recargar();
            }

            // Actualizamos la munición agregada (debería ser > 0 ahora)
            actualizarMunicionGrupo();
        }
        break;
    }

    case EstadoRotacion::EsperandoOrden:
    {
        // Grupo replegado y recargado, esperando a que Nivel lo
        // "llame" para volver al frente.
        accionMoverActiva    = false;
        accionDispararActiva = false;
        // El "llamadoPorOtraOleada" ya se procesó al inicio de la función
        break;
    }
    }
}

bool OleadaCadetes::todosEnCampamento() const
{
    // Si no hay puntos definidos, consideramos que no hay nada que comprobar.
    if (puntosCampamento.empty())
        return true;

    const qreal radioLlegadaCamp  = 25.0;
    const qreal radioLlegadaCamp2 = radioLlegadaCamp * radioLlegadaCamp;

    for (FuerzaArmada *e : grupo) {
        if (!e || e->estaMuerto())
            continue;

        Vector2D posScene(e->scenePos());

        // Buscar el punto de campamento más cercano
        qreal minDist2 = std::numeric_limits<qreal>::max();
        for (const auto &p : puntosCampamento) {
            qreal d2 = (posScene - p).magnitud2();
            if (d2 < minDist2)
                minDist2 = d2;
        }

        // Si este cadete está demasiado lejos de todos los puntos, aún no estamos "en campamento"
        if (minDist2 > radioLlegadaCamp2)
            return false;
    }

    return true;
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
        // Dirección base hacia el foco (jugador, campamento, retirada...)
        if( modo == ModoGrupo::AtaqueDirecto ) dirBase = calcularDireccionHaciaJugador(e);
        else if ( modo == ModoGrupo::Rotacion && estadoRotacion == EstadoRotacion::Atacando ) dirBase = calcularDireccionHaciaJugador(e);
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
    // Asumimos que focoMovimiento está en coordenadas locales
    Vector2D centro = calcularCentroGrupoScene();

    Vector2D pos_= Vector2D::nulo();
    if(e != nullptr) pos_ = Vector2D(e->scenePos());
    else pos_ = centro;
    Vector2D toTarget = pos_*-1.0;

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

// Separación con respecto a otros cadetes del grupo
Vector2D OleadaCadetes::calcularSeparacion(FuerzaArmada *e) const
{
    const qreal radioSep  = 40.0;
    const qreal radioSep2 = radioSep * radioSep;

    Vector2D posScene(e->scenePos());
    Vector2D dirRef = e->getDireccion();
    if (dirRef.magnitud2() > 0.0)
        dirRef = dirRef.normalizado();

    Vector2D acumulado = Vector2D::nulo();

    for (FuerzaArmada *otro : grupo) {
        if (!otro || otro == e || otro->estaMuerto())
            continue;

        Vector2D posOther(otro->scenePos());
        Vector2D diff = posScene - posOther;
        qreal dist2 = diff.magnitud2();

        if (dist2 < 1e-3) {
            // Están casi encima: empuje perpendicular a la dirección de referencia
            if (dirRef.magnitud2() > 0.0) {
                Vector2D perp(-dirRef.y(), dirRef.x());
                if (perp.magnitud2() > 0.0) {
                    perp = perp.normalizado();
                    acumulado += perp * 5.0;
                }
            }
        }
        else if (dist2 < radioSep2) {
            // Separación suave proporcional a 1/distancia
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
//  HELPERS GEOMÉTRICOS / COLISIÓN
// ===================================================

// Centro del grupo en coordenadas de escena
Vector2D OleadaCadetes::calcularCentroGrupoScene() const
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

// Reacción al chocar con un obstáculo (misma idea que tu react_colision)
Vector2D OleadaCadetes::reaccionarColision(Obstaculo *obs,
                                           FuerzaArmada *cadet,
                                           qreal step)
{
    // Pos local actual del cadete
    Vector2D pos_act(cadet->pos().x(), cadet->pos().y());

    Vector2D dir_act = cadet->getDireccion();
    if (dir_act.magnitud2() == 0.0)
        return pos_act;

    // "Normal" aproximada usando la posición del obstáculo (simplificado)
    Vector2D dir_normal_obs(obs->scenePos());
    dir_normal_obs = dir_normal_obs.normalizado().getNormal();

    qreal dot = dir_act.dot(dir_normal_obs);
    dir_act = dir_act.normalizado();

    Vector2D new_pos;

    if (dot >= 0)
        new_pos = pos_act + (dir_act - dir_normal_obs).normalizado() * step;
    else
        new_pos = pos_act + (dir_act + dir_normal_obs).normalizado() * step;

    return new_pos;
}

bool OleadaCadetes::disponibleParaRelevo() const
{
    // Solo tiene sentido en modo Rotación
    if (modo != ModoGrupo::Rotacion)
        return false;

    // Sin enemigos, no hay a quién mandar
    if (enemigosRestantes <= 0)
        return false;

    // Disponible si ya está en fase EsperandoOrden (ya replegado y recargado)
    return (estadoRotacion == EstadoRotacion::EsperandoOrden);
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
            estadoRotacion == EstadoRotacion::Huyendo &&
            enemigosRestantes > 0);
}

void OleadaCadetes::marcarLlamadoPorOtraOleada()
{
    // Solo tiene sentido marcar apoyo si el grupo no está huyendo ni muerto
    if (enemigosRestantes <= 0) return;
    if (estadoRotacion == EstadoRotacion::Huyendo) return;

    llamadoPorOtraOleada = true;
}

Vector2D OleadaCadetes::centroGrupoScene() const
{
    return calcularCentroGrupoScene();
}
