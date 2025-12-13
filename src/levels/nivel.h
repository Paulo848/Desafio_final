#ifndef NIVEL_H
#define NIVEL_H

// ============================
// Qt: includes
// ============================
#include <QWidget>
#include <QRectF>

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
    Vector2D indiceGrid;   // (fila, columna)
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
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // --- Loop y acciones de UI ---
    void actualizarJuego();
    void onVolverClicked();
    void onSonidoAmbienteTerminado();

private:
    bool debugChunks = false;
    void actualizarDebugChunksVisibles();

    // --- Debug de zoom ---
    qreal zoomDefault = 2.0;
    qreal zoomActual  = 2.0;
    qreal zoomMin     = 0.1;
    qreal zoomMax     = 2.5;
    qreal zoomStep    = 1.10;

    void aplicarZoomVista();

    // ============================
    //  Datos de estado
    // ============================
    int     numNivel;
    Vector2D viewportSize;
    Vector2D fondoSize;
    Vector2D limiteMapa;
    Vector2D camara;
    Vector2D mouseDir;

    // Escena y vista
    QGraphicsView       *vista       = nullptr;
    QGraphicsScene      *escena      = nullptr;
    QGraphicsPixmapItem *fondoScroll = nullptr;

    // Timers
    QTimer *timer = nullptr;

    // Entidades
    Cadete *jugador = nullptr;
    std::vector<Cadete*>    enemigos;
    std::vector<Obstaculo*> obstaculos;
    std::vector<Agente*>    agentes;
    std::vector<Proyectil*> proyectiles;

    // Oleadas
    int ronda_act    = 1;
    int total_rondas = 3;

    void coordinarRotacionRonda3();
    OleadaCadetes* encontrarAliadoMasCercanoEnRotacion(OleadaCadetes *petidora);

    // Input movimiento
    bool m_moveLeft  = false;
    bool m_moveRight = false;
    bool m_moveUp    = false;
    bool m_moveDown  = false;

    // HUD
    QWidget      *hud        = nullptr;
    QPushButton  *btnVolver  = nullptr;
    QLabel       *lblEnemigos = nullptr;
    QLabel       *lblRonda    = nullptr;
    QLabel       *lblBalas    = nullptr;
    QProgressBar *barraVida   = nullptr;

    // --- Recarga del jugador ---
    QProgressBar *barraRecarga       = nullptr;
    bool          recargandoJugador  = false;
    int           recargaTicksActual = 0;
    int           recargaTicksTotal  = 60;

    // --- Game Over / End Game ---
    bool          juegoTerminado    = false;
    QWidget      *overlayGameOver   = nullptr;
    QLabel       *lblGameOverTitulo = nullptr;
    QLabel       *lblGameOverStats  = nullptr;
    QPushButton  *btnReintentar     = nullptr;
    QPushButton  *btnMenuGameOver   = nullptr;

    void finJuegoPorMuerte();
    void finJuegoPorVictoria();
    void mostrarGameOverOverlay(bool victoria = false);
    void destruirOverlayGameOver();

    // ============================
    //  Recarga jugador
    // ============================
    void iniciarRecargaJugador();
    void cancelarRecargaJugador();
    void actualizarRecargaJugador();

    // ============================
    //  Chunks del mapa
    // ============================
    int   numColsChunks  = 5;
    int   numFilasChunks = 0;
    qreal chunkWidth     = 0.0;
    qreal chunkHeight    = 0.0;

    std::vector<Chunk> chunks;
    void inicializarChunks();

    // ============================
    //  Inicialización
    // ============================
    void inicializarUI();
    void inicializarEscena();
    void cargarElementosNivel();

    // ============================
    //  Loop de juego / lógica
    // ============================
    void actualizarFondo();
    void actualizarPosicionFondo();
    void actualizarIA();
    void actualizarOleadas();
    void avanzarProyectiles();
    void limpiarProyectilesMuertos();
    void desactivarEnemigosMuertos();
    void actualizarHUD();

    bool jugadorTocaObstaculo() const;

    // ============================
    //  Obstáculos
    // ============================
    Obstaculo* crearObstaculoEnChunk(Chunk &chunk,
                                     qreal refHalfSize,
                                     qreal xRef,
                                     qreal yRef,
                                     int cuadrante,
                                     const QString &spriteName);

    void colocarObstaculosPatron1(Chunk &chunk);
    void colocarObstaculosPatron2(Chunk &chunk);
    void colocarObstaculosPatron3(Chunk &chunk);
    void poblarObstaculosPatronCiclico();

    // ============================
    //  Helpers de oleadas
    // ============================
    bool existeRonda(int r) const;
    void crearRonda1();
    void crearRonda2();
    void crearRonda3();
    void activarGruposRondaActual();
    void avanzarRondaSiCompleta();

    void sinergiaEmboscadaRonda2();

    // Sonido
    QSoundEffect *m_sonidoAmbiente = nullptr;
    void cargarSonidos();
    void cargarSonidosDesdeArchivos();
};

#endif // NIVEL_H
