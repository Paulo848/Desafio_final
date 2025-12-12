#ifndef OLEADACADETES_H
#define OLEADACADETES_H

#include <vector>
#include <qmath.h>

#include "agente.h"
#include "vector2d.h"

class Cadete;
class Obstaculo;

// Controla las oleadas de cadetes (modos: AtaqueDirecto, Campamento, Rotación)
class OleadaCadetes : public Agente
{
public:
    explicit OleadaCadetes(Nivel *nivelPtr);
    ~OleadaCadetes() override = default;

    // ============================
    //  Implementación de Agente
    // ============================

    // ¿Este grupo está en fase de huida dentro del modo Rotación?
    bool estaHuyendo() const;

    // Marca que este grupo ha sido llamado por otra oleada para apoyar/atacar
    void marcarLlamadoPorOtraOleada();

    // Centro del grupo en coordenadas de escena (para calcular distancias)
    Vector2D centroGrupoScene() const;

    void inicializarRondas(int total) override;

    // Spawn de una ronda alrededor del jugador
    void spawnRonda(int cantidad,
                    qreal radioMin, qreal radioMax,
                    qreal angMinRad, qreal angMaxRad) override;

    // Tick principal por frame
    void actualizar() override;

    // Fin de la ronda asignada a este agente
    bool rondaCompletada() const override;

    // ============================
    //  Configuración desde Nivel
    // ============================

    // Slots de campamento (alrededor de un punto base)
    void setPuntosCampamento(const std::vector<Vector2D> &puntos);

    // Punto de retirada para el modo Rotación
    void setPuntoRetirada(const Vector2D &p);

    // Radio que activa el ataque cuando el jugador se acerca
    void setRadioActivacion(qreal r);

    // Flags para que Nivel coordine rotación entre oleadas
    inline bool estaPidiendoRefuerzo() const { return pidiendoRefuerzo; }
    inline void limpiarPeticionRefuerzo()    { pidiendoRefuerzo = false; }
    // --- API específica de Rotación para que Nivel coordine relevo ---
    // Devuelve true si este grupo está en modo Rotación, con enemigos vivos
    // y ya replegado/recargado esperando que lo llamen.
    bool disponibleParaRelevo() const;

private:
    // ============================
    //  Estado interno Rotación
    // ============================

    enum class EstadoRotacion {
        EnCampamento,   // está en su posición de campamento
        Atacando,       // presionando al jugador
        Huyendo,        // retirándose a punto de retirada
        EsperandoOrden  // ya recargada, esperando que Nivel la mande otra vez
    };

    // --- Layout / posiciones ---
    std::vector<Vector2D> puntosCampamento;  // slots donde se ubican los cadetes
    Vector2D puntoRetirada;                  // punto lejos del jugador
    qreal    radioActivacion = 0.0;          // distancia a la que reaccionan al jugador

    // --- Acciones básicas activables ---
    bool     accionMoverActiva    = false;   // “ir hacia focoMovimiento”
    bool     accionDispararActiva = false;   // “tener disparar(true) en Cadete”
    Vector2D focoMovimiento;                 // destino (jugador, campamento, retirada)

    // --- Estado de rotación / coordinación ---
    EstadoRotacion estadoRotacion = EstadoRotacion::EnCampamento;
    bool pidiendoRefuerzo         = false;   // esta oleada pide relevo
    bool llamadoPorOtraOleada     = false;   // esta oleada fue llamada para atacar

    // --- Cuantos cadetes del grupo tienen balas ---

    int municionGrupoActual = 0;
    // Cooldown compartido para los disparos de este grupo
    int ticksDesdeUltDisparo;

    // Cada cuántos "ticks" del juego se permite una ráfaga del grupo
    static const int COOLDOWN_DISPARO_TICKS;

    // ============================
    //  Orquestador por modo
    // ============================

    // Traduce ModoGrupo::AtaqueDirecto en flags/acciones básicas
    void actualizarModoAtaqueDirecto();

    // Traduce ModoGrupo::Campamento en flags/acciones básicas
    void actualizarModoCampamento();

    // Traduce ModoGrupo::Rotacion en flags/acciones básicas + estadoRotacion
    void actualizarModoRotacion();

    // ============================
    //  Aplicación de acciones
    // ============================

    // Aplica “ir hacia focoMovimiento” a todos los cadetes vivos
    void aplicarAccionMover();

    // Aplica “disparar on/off + recargas” según accionDispararActiva
    void aplicarAccionDisparar();

    // Recalcula munición total del grupo (llamando a Cadete::tieneMunicion)
    void actualizarMunicionGrupo();

    // Decide si se disparan balas en este frame
    void actualizarDisparos();

    // ============================
    //  Helpers de movimiento / steering
    // ============================

    // Dirección desde el cadete hasta focoMovimiento
    Vector2D calcularDireccionHaciaFoco(FuerzaArmada *e = nullptr) const;
    Vector2D calcularDireccionHaciaJugador(FuerzaArmada *e = nullptr) const;

    // Vector de separación para no montarse sobre otros cadetes
    Vector2D calcularSeparacion(FuerzaArmada *e) const;

    // Combina dirección base + separación
    Vector2D combinarDirecciones(const Vector2D &base,
                                 const Vector2D &sep) const;

    // Aplica un movimiento con colisiones contra obstáculos / otros
    void moverUnidadConColisiones(FuerzaArmada *e,
                                  const Vector2D &dir,
                                  qreal step);

    // ============================
    //  Helpers de geometría / colisión
    // ============================

    // Centro del grupo en coordenadas de escena
    Vector2D calcularCentroGrupoScene() const;

    // Reacción al chocar con un obstáculo (igual a tu react_colision actual)
    Vector2D reaccionarColision(Obstaculo *obs,
                                FuerzaArmada *cadet,
                                qreal step);

    // ============================
    //  Utilidades internas de estado
    // ============================

    // Recuenta enemigos vivos/muertos y marca activo/Muerto
    void actualizarEstadoVivosYMuertos();

    // (Opcional) para saber si ya todos llegaron a sus slots de campamento
    bool todosEnCampamento() const;
};

#endif // OLEADACADETES_H
