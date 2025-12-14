#ifndef NIVEL_H
#define NIVEL_H

// ============================
// Qt: includes
// ============================
#include <QWidget>
#include <QRectF>
#include <QPointF>

#include <vector>
#include <QString>

// ============================
// Forward declarations (Qt)
// ============================
class QTimer;
class QKeyEvent;
class QMouseEvent;
class QEvent;
class QResizeEvent;

class QGraphicsView;
class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsRectItem;

class QSoundEffect;

class QPushButton;
class QLabel;
class QProgressBar;

// ============================
// Forward declarations (juego)
// ============================
class Cadete;
class Obstaculo;
class Agente;
class Proyectil;
class OleadaCadetes;

#include "vector2d.h"

// ----------------------------------------
//  Estructura de chunk de mapa
// ----------------------------------------
struct Chunk
{
    Vector2D indiceGrid;   // (col, fila) en tu código (Vector2D(col, fila))
    QRectF   area;         // rect en coords locales del fondo
    Vector2D centro;       // centro del chunk (local fondo)

    std::vector<Obstaculo*> obstaculos;

    QGraphicsRectItem* debugRect = nullptr;
};

class Nivel : public QWidget
{
    Q_OBJECT

public:
    explicit Nivel(int numeroNivel,
                   QWidget *parent = nullptr,
                   qreal _v_alto = 700,
                   qreal _v_ancho = 1100);
    ~Nivel() override;

    // --- API pública sencilla ---
    void disparar(Cadete *emisor);

    inline QGraphicsPixmapItem* getFondoScroll() const { return fondoScroll; }
    inline QGraphicsScene*      getEscena()      const { return escena; }
    inline Vector2D             getfondoSize()   const { return fondoSize; }
    Vector2D                    getJugDir()      const;

    inline void registrarEnemigo(Cadete *e) { if (e) enemigos.push_back(e); }
    inline void registrarAgente(Agente *a)  { if (a) agentes.push_back(a); }

    inline const std::vector<Agente*>& getAgentes() const { return agentes; }
    inline const std::vector<Chunk>&   getChunks()  const { return chunks; }

signals:
    void volverAlMenu();
    void reintentarNivel(int numNivel);

protected:
    // --- Entradas ---
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // --- Loop y acciones de UI ---
    void actualizarJuego();
    void onVolverClicked();
    void onSonidoAmbienteTerminado();

private:

    // ============================
    //  Datos de estado
    // ============================
    int     numNivel;
    Vector2D viewportSize;
    Vector2D fondoSize;
    Vector2D limiteMapa;
    Vector2D camara;
    Vector2D mouseDir;

    // Oleadas
    int ronda_act    = 1;
    int total_rondas = 3;

    // Input movimiento
    bool m_moveLeft  = false;
    bool m_moveRight = false;
    bool m_moveUp    = false;
    bool m_moveDown  = false;

    // Sonido
    QSoundEffect *m_sonidoAmbiente = nullptr;

    // ============================
    // Entidades
    // ============================
    Cadete *jugador = nullptr;
    std::vector<Cadete*>    enemigos;
    std::vector<Obstaculo*> obstaculos;
    std::vector<Agente*>    agentes;
    std::vector<Proyectil*> proyectiles;
    std::vector<Chunk> chunks;

    // Recarga del jugador
    QProgressBar *barraRecarga       = nullptr;
    bool          recargandoJugador  = false;
    int           recargaTicksActual = 0;
    int           recargaTicksTotal  = 60;
    void iniciarRecargaJugador();
    void cancelarRecargaJugador();

    // End game
    bool          juegoTerminado    = false;
    QWidget      *overlayGameOver   = nullptr;
    QLabel       *lblGameOverTitulo = nullptr;
    QLabel       *lblGameOverStats  = nullptr;
    QPushButton  *btnReintentar     = nullptr;
    QPushButton  *btnMenuGameOver   = nullptr;

    // ============================
    //  Chunks del mapa + zoom
    // ============================
    int   numColsChunks  = 5;
    int   numFilasChunks = 0;
    qreal chunkWidth     = 0.0;
    qreal chunkHeight    = 0.0;

    bool debugChunks = false;
    void actualizarDebugChunksVisibles();
    qreal zoomDefault = 2.0;
    qreal zoomActual  = 2.0;
    qreal zoomMin     = 0.1;
    qreal zoomMax     = 2.5;
    qreal zoomStep    = 1.10;
    void aplicarZoomVista();

    // ============================
    //  UI
    // ============================

    // Escena y vista
    QGraphicsView       *vista       = nullptr;
    QGraphicsScene      *escena      = nullptr;
    QGraphicsPixmapItem *fondoScroll = nullptr;

    // HUD
    QWidget      *hud         = nullptr;
    QPushButton  *btnVolver   = nullptr;
    QLabel       *lblEnemigos = nullptr;
    QLabel       *lblRonda    = nullptr;
    QLabel       *lblBalas    = nullptr;
    QProgressBar *barraVida   = nullptr;

    // Timer principal
    QTimer *timer = nullptr;

    // ============================
    //  Setup
    // ============================
    void setupNivel();

    // 1) Mapa / Fondo
    void setupMapaYFondo();
    void crearEscenaYVista();
    void configurarFondoSegunNivel();
    void cargarFondo();
    void configurarCamaraInicial();
    void configurarVistaInput();

    // 2) Chunks + Obstáculos
    void setupChunksYObstaculos();
    void configurarGridChunks();
    void crearChunks();

    void poblarObstaculosPatronCiclico();
    void colocarObstaculosPatron1(Chunk &chunk);
    void colocarObstaculosPatron2(Chunk &chunk);
    void colocarObstaculosPatron3(Chunk &chunk);
    Obstaculo* crearObstaculoEnChunk(Chunk &chunk,
                                     qreal refHalfSize,
                                     qreal xRef,
                                     qreal yRef,
                                     int cuadrante,
                                     const QString &spriteName);

    // 3) Entidades
    void setupEntidades();

    // 4) HUD
    void setupHUD();
    void crearHUDRoot();
    void crearPanelSuperiorHUD();
    void crearBarraRecargaHUD();
    void crearBotonVolverHUD();

    // 5) Audio
    void setupAudio();
    void cargarAudioAmbiente();
    void conectarLoopAudioAmbiente();

    // 6) Loop principal
    void setupLoopPrincipal();

    // Post-setup
    void aplicarSetupPost();

    // ============================
    //  Loop de juego / lógica
    // ============================
    // 2.1 Movimiento del jugador
    void actualizarMovimiento();
    void actualizarCamara();
    void actualizarPosicionFondo();
    void resolverColisionMovimiento(const Vector2D& camaraAnterior, const QPointF& posFondoAnterior);

    // 2.2 OLEADAS
    void actualizarIA();

    // 2.3) Rondas
    void actualizarOleadas();
    bool asegurarRondaCreada();

    // Creación de rondas
    void crearRonda1();
    void crearRonda2();
    void crearRonda3();

    void activarGruposRondaActual();
    void avanzarRondaSiCompleta();

    // “Reglas especiales” por ronda
    void actualizarRonda();
    void actualizarRonda2();
    void actualizarRonda3();

    // 2.4) Proyectiles
    void avanzarProyectiles();
    void limpiarProyectilesMuertos();

    // 2.5) Limpieza enemigos
    void desactivarEnemigosMuertos();

    // 2.6) Recarga
    void actualizarRecargaJugador();

    // 2.7) HUD
    void actualizarHUD();

    // 2.8) Condición de muerte
    void finJuegoPorMuerte();
    void finJuegoPorVictoria();
    void mostrarGameOverOverlay(bool victoria = false);
    void destruirOverlayGameOver();

    bool jugadorTocaObstaculo() const;

    //  Helpers
    bool existeRonda(int r) const;    
    void destruirAgentesDeRonda(int r);
    OleadaCadetes* encontrarAliadoMasCercanoEnRotacion(OleadaCadetes *petidora);
};

#endif // NIVEL_H
