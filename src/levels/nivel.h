#ifndef NIVEL_H
#define NIVEL_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QKeyEvent>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QSoundEffect>
#include <QRectF>
#include <vector>
#include <QString>

#include "vector2d.h"
#include "fuerzaarmada.h"
#include "cadete.h"
#include "bala.h"
#include "obstaculo.h"
#include "agente.h"
#include "oleadacadetes.h"

class QGraphicsRectItem;

// ----------------------------------------
//  Estructura de chunk de mapa
// ----------------------------------------
struct Chunk
{
    Vector2D indiceGrid;           // (fila, columna)
    QRectF   area;                 // rect en coords locales del fondo
    Vector2D centro;               // centro del chunk (local fondo)

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
    inline Vector2D             getJugDir()      const { return jugador->getDireccion(); }

    inline void registrarEnemigo(Cadete *e) { if (e) enemigos.push_back(e); }

    inline void registrarAgente(Agente *a) { if (a) agentes.push_back(a); }

    inline const std::vector<Agente*>& getAgentes() const { return agentes; }

    // Acceso opcional a la grilla de chunks
    inline const std::vector<Chunk>& getChunks() const { return chunks; }

signals:
    void volverAlMenu();

protected:
    // --- Entradas ---
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // --- Loop y acciones de UI ---
    void actualizarJuego();
    void disparosEnemigos();
    void onVolverClicked();
    void onSonidoAmbienteTerminado();

private:

    bool debugChunks = true;   // por ahora true para verlos siempre

    // ============================
    //  Datos de estado
    // ============================

    // Configuración del nivel
    int     numNivel;
    Vector2D viewportSize;
    Vector2D fondoSize;
    Vector2D limiteMapa;
    Vector2D camara;
    Vector2D mouseDir;

    // Escena y vista
    QGraphicsView       *vista;
    QGraphicsScene      *escena;
    QGraphicsPixmapItem *fondoScroll;

    // Timers
    QTimer *timer;
    QTimer *timerDisparoEnemigos;

    // Entidades
    Cadete *jugador;
    std::vector<Cadete*>      enemigos;
    std::vector<Obstaculo*>   obstaculos;
    std::vector<Agente*>      agentes;
    std::vector<Proyectil*>   proyectiles;

    // Oleadas
    int ronda_act;
    int total_rondas;
    void coordinarRotacionRonda3();
    OleadaCadetes* encontrarAliadoMasCercanoEnRotacion(OleadaCadetes *petidora);

    // Input movimiento
    bool m_moveLeft;
    bool m_moveRight;
    bool m_moveUp;
    bool m_moveDown;

    // HUD
    QWidget      *hud;
    QPushButton  *btnVolver;
    QLabel       *lblEnemigos;
    QLabel       *lblRonda;
    QLabel       *lblBalas;
    QProgressBar *barraVida;

    // ============================
    //  Chunks del mapa
    // ============================

    int   numColsChunks;   // número de columnas de chunks (lo decides tú)
    int   numFilasChunks;  // se calcula a partir del alto del fondo
    qreal chunkWidth;      // ancho de cada chunk (en coords del fondo)
    qreal chunkHeight;     // alto de cada chunk (en coords del fondo)

    std::vector<Chunk> chunks;

    // Construye la grilla de chunks usando fondoSize, numColsChunks, etc.
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
    void manejarColisiones();

    // Colisiones / consultas
    bool jugadorTocaObstaculo() const;

    // ============================
    //  Obstáculos
    // ============================

    void crearObstaculosFijos();
    void crearObstaculosAleatorios(int numExtraObst,
                                   qreal radioMin,
                                   qreal radioMax);

    Obstaculo* crearObstaculoEnChunk(Chunk &chunk,
                                     qreal refHalfSize,
                                     qreal xRef,
                                     qreal yRef,
                                     int cuadrante,
                                     const QString &spriteName);

    // Patrones de obstáculos por tipo de chunk (1, 2 y 3)
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

    void sinergiaEmboscadaRonda2();  // (probablemente la dejes de usar)

    //sonido
    QSoundEffect *m_sonidoAmbiente;

    void cargarSonidos();
    void cargarSonidosDesdeArchivos();


};

#endif // NIVEL_H
