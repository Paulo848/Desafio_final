#include "nivel.h"
#include "cadete.h"
#include "fuerzaarmada.h"
#include "obstaculo.h"
#include "constantes_juego.h"

#include <QVBoxLayout>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QBrush>
#include <QPixmap>
#include <QDebug>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <algorithm>
#include <QHBoxLayout>
#include <QFrame>

#include "bala.h"
#include "oleadacadetes.h"
#include <limits>

// ============================
// 1) Constructor / Destructor
// ============================

Nivel::Nivel(int numeroNivel, QWidget *parent, qreal _v_alto, qreal _v_ancho)
    : QWidget(parent),
    numNivel(numeroNivel),
    viewportSize(_v_ancho, _v_alto),
    fondoSize(0.0, 0.0),
    limiteMapa(fondoSize - viewportSize),
    camara(0.0, 0.0),
    vista(nullptr),
    escena(nullptr),
    fondoScroll(nullptr),
    timer(nullptr),
    timerDisparoEnemigos(nullptr),
    jugador(nullptr),
    mouseDir(0.0, 0.0),
    m_moveLeft(false),
    m_moveRight(false),
    m_moveUp(false),
    m_moveDown(false),
    ronda_act(1),
    total_rondas(3),
    m_sonidoAmbiente(nullptr),
    numColsChunks(5),
    numFilasChunks(0),
    chunkWidth(0.0),
    chunkHeight(0.0)
{
    // Setup UI/escena/nivel
    inicializarUI();
    inicializarEscena();
    cargarElementosNivel();

    // Cargar sonidos
    cargarSonidos();

    //Aplicar Zoom por defecto
    aplicarZoomVista();

    // Timer principal
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Nivel::actualizarJuego);
    timer->start(16);
}

Nivel::~Nivel()
{
    if (timer) timer->stop();

    // Limpia sonidos
    if (m_sonidoAmbiente) {
        m_sonidoAmbiente->stop();
        delete m_sonidoAmbiente;
        m_sonidoAmbiente = nullptr;
    }

    // Borrar enemigos
    for (auto e : enemigos) delete e;
    enemigos.clear();

    // Agentes
    for (auto a : agentes) delete a;
    agentes.clear();

    // Jugador
    delete jugador;

    // Obstáculos: son hijos de fondoScroll → los destruye la escena
    obstaculos.clear();

    // Chunks: solo limpiamos el vector (no hay new dentro de Chunk)
    chunks.clear();

    destruirOverlayGameOver();

}

// ============================
// 2) Inicialización de escena
// ============================

void Nivel::inicializarUI()
{
    // Crear vista y escena
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);

    vista  = new QGraphicsView(this);
    escena = new QGraphicsScene(this);
    vista->setTransformationAnchor(QGraphicsView::AnchorViewCenter);

    // Rectángulo de escena
    escena->setSceneRect(-viewportSize.x()/2.0,
                         -viewportSize.y()/2.0,
                         viewportSize.x(),
                         viewportSize.y());

    // Fondo del nivel
    QPixmap imagenFondo(":/ui/nivel_3/fondo_nivel_3.png");
    fondoSize.set(imagenFondo.width(), imagenFondo.height());
    limiteMapa = fondoSize - viewportSize;
    if (limiteMapa.x() < 0) limiteMapa.setX(0);
    if (limiteMapa.y() < 0) limiteMapa.setY(0);
    camara = limiteMapa / 2.0;

    fondoScroll = new QGraphicsPixmapItem(imagenFondo);
    fondoScroll->setZValue(-1000);
    actualizarPosicionFondo();
    escena->addItem(fondoScroll);

    // ---------------------------------
    //  Configuración básica de chunks
    // ---------------------------------
    // numColsChunks viene con un valor por defecto (p.ej. 5),
    // pero nos aseguramos de que sea al menos 1.
    if (numColsChunks < 1)
        numColsChunks = 1;

    // Ancho del chunk a partir del ancho del fondo
    chunkWidth  = fondoSize.x() / static_cast<qreal>(numColsChunks);

    // Por ahora queremos chunks cuadrados
    chunkHeight = chunkWidth;

    // Cantidad de filas según el alto del fondo
    // Cantidad de filas según el alto del fondo,
    // con regla del decimal: si la parte fraccionaria >= 0.75
    // añadimos una fila extra.
    if (chunkHeight > 0.0) {
        qreal divFilas = fondoSize.y() / chunkHeight;   // p.ej. 4.2, 4.8, etc.
        int  filasBase = static_cast<int>(divFilas);    // parte entera (floor)
        qreal frac     = divFilas - filasBase;          // parte decimal

        numFilasChunks = filasBase;

        // Si el decimal es >= 0.75 → creamos la fila extra
        if (frac >= 0.75)
            ++numFilasChunks;

        // Seguridad: si hay fondo y por redondeos quedó en 0, forzamos al menos 1 fila
        if ((numFilasChunks < 1) && fondoSize.y() > 0.0)
            numFilasChunks = 1;

        qDebug() << "Chunks filas - div:" << divFilas
                 << "base:" << filasBase
                 << "frac:" << frac
                 << "numFilasChunks:" << numFilasChunks;
    } else {
        numFilasChunks = 0;
    }

    inicializarChunks();

    poblarObstaculosPatronCiclico();

    // Config vista
    vista->setScene(escena);
    vista->setFocusPolicy(Qt::NoFocus);
    vista->setMouseTracking(true);
    vista->viewport()->setMouseTracking(true);
    vista->viewport()->installEventFilter(this);
    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setMouseTracking(true);

    // HUD bonito
    hud = new QWidget(this);
    hud->setObjectName("hudRoot");
    hud->setAttribute(Qt::WA_TransparentForMouseEvents);
    hud->setGeometry(0, 0, viewportSize.x(), viewportSize.y());

    hud->setStyleSheet(
        "QWidget#hudRoot {"
        "   background: transparent;"
        "}"
        "QFrame#hudPanel {"
        "   background-color: rgba(5, 5, 10, 190);"
        "   border-radius: 12px;"
        "}"
        "QLabel {"
        "   color: #f0f0f0;"
        "}"
        "QProgressBar {"
        "   background-color: rgba(255, 255, 255, 40);"
        "   border-radius: 8px;"
        "   border: 1px solid rgba(0, 0, 0, 160);"
        "   text-align: center;"
        "   font-size: 11px;"
        "   font-weight: 600;"
        "   color: #f0f0f0;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #2ecc71;"
        "   border-radius: 8px;"
        "}"
        );

    // Panel superior (lo posicionamos a mano)
    QFrame *panelSuperior = new QFrame(hud);
    panelSuperior->setObjectName("hudPanel");

    auto *panelLayout = new QHBoxLayout(panelSuperior);
    panelLayout->setContentsMargins(12, 8, 12, 8);
    panelLayout->setSpacing(24);

    // --- Columna VIDA ---
    auto *colVida = new QVBoxLayout();
    QLabel *lblVidaTitulo = new QLabel("VIDA", panelSuperior);
    lblVidaTitulo->setStyleSheet("font-size: 11px; font-weight: 600; color: #c0c0c0;");
    barraVida = new QProgressBar(panelSuperior);
    barraVida->setMinimum(0);
    barraVida->setMaximum(100);
    barraVida->setValue(100);
    barraVida->setFixedWidth(180);
    colVida->addWidget(lblVidaTitulo);
    colVida->addWidget(barraVida);
    panelLayout->addLayout(colVida);

    // --- Columna RONDA ---
    auto *colRonda = new QVBoxLayout();
    QLabel *lblRondaTitulo = new QLabel("RONDA", panelSuperior);
    lblRondaTitulo->setStyleSheet("font-size: 11px; font-weight: 600; color: #c0c0c0;");
    lblRonda = new QLabel("1 / 3", panelSuperior);
    lblRonda->setStyleSheet("font-size: 14px; font-weight: 700;");
    colRonda->addWidget(lblRondaTitulo);
    colRonda->addWidget(lblRonda);
    panelLayout->addLayout(colRonda);

    // --- Columna ENEMIGOS ---
    auto *colEnemigos = new QVBoxLayout();
    QLabel *lblEnemigosTitulo = new QLabel("ENEMIGOS", panelSuperior);
    lblEnemigosTitulo->setStyleSheet("font-size: 11px; font-weight: 600; color: #c0c0c0;");
    lblEnemigos = new QLabel("-- / --", panelSuperior);
    lblEnemigos->setStyleSheet("font-size: 14px; font-weight: 700;");
    colEnemigos->addWidget(lblEnemigosTitulo);
    colEnemigos->addWidget(lblEnemigos);
    panelLayout->addLayout(colEnemigos);

    // --- Columna MUNICIÓN ---
    auto *colBalas = new QVBoxLayout();
    QLabel *lblBalasTitulo = new QLabel("MUNICIÓN", panelSuperior);
    lblBalasTitulo->setStyleSheet("font-size: 11px; font-weight: 600; color: #c0c0c0;");
    lblBalas = new QLabel("-- / --", panelSuperior);
    lblBalas->setStyleSheet("font-size: 14px; font-weight: 700;");
    colBalas->addWidget(lblBalasTitulo);
    colBalas->addWidget(lblBalas);
    panelLayout->addLayout(colBalas);

    //hudRootLayout->addWidget(panelSuperior, 0, Qt::AlignTop | Qt::AlignLeft);
    //hudRootLayout->addStretch();

    //hudRootLayout->addWidget(panelSuperior, 0, Qt::AlignTop | Qt::AlignLeft);
    //hudRootLayout->addStretch();

    // ------------------------------
    // Barra de recarga (centro abajo)
    // ------------------------------
    barraRecarga = new QProgressBar(hud);
    barraRecarga->setRange(0, recargaTicksTotal);
    barraRecarga->setValue(0);
    barraRecarga->setVisible(false);
    barraRecarga->setTextVisible(true);
    barraRecarga->setFormat("Recargando... %p%");

    int anchoRecarga = 220;
    int altoRecarga  = 18;
    int xRecarga = static_cast<int>((viewportSize.x() - anchoRecarga) / 2.0);
    int yRecarga = static_cast<int>(viewportSize.y() - 70);

    barraRecarga->setGeometry(xRecarga, yRecarga, anchoRecarga, altoRecarga);

    // Botón volver (manteniendo posición en esquina inferior izquierda)
    btnVolver = new QPushButton("Volver al Menú", this);
    btnVolver->setGeometry(20, viewportSize.y() - 50, 150, 30);
    btnVolver->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(5, 5, 10, 190);"
        "   color: #f0f0f0;"
        "   border-radius: 8px;"
        "   padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(40, 40, 60, 220);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(20, 20, 35, 240);"
        "}"
        );
    connect(btnVolver, &QPushButton::clicked, this, &Nivel::onVolverClicked);

    // Ajustar tamaño del panel según su contenido
    panelSuperior->adjustSize();
    // Moverlo a la esquina superior izquierda con un margen
    panelSuperior->move(16, 16);

    // Final UI
    vista->centerOn(0,0);
    layout->addWidget(vista);
    setLayout(layout);
    setFocusPolicy(Qt::StrongFocus);
}

void Nivel::inicializarChunks()
{
    chunks.clear();

    // Seguridad básica
    if (numColsChunks <= 0 || numFilasChunks <= 0 ||
        chunkWidth <= 0.0      || chunkHeight <= 0.0) {
        qDebug() << "inicializarChunks(): parámetros inválidos, no se crean chunks";
        return;
    }

    chunks.reserve(static_cast<size_t>(numColsChunks * numFilasChunks));

    for (int fila = 0; fila < numFilasChunks; ++fila) {
        for (int col = 0; col < numColsChunks; ++col) {

            Chunk c;

            // (columna, fila) → usamos x=col, y=fila
            c.indiceGrid = Vector2D(col, fila);

            // Origen del rectángulo del chunk en coords LOCALES del fondo
            qreal x0 = static_cast<qreal>(col)  * chunkWidth;
            qreal y0 = static_cast<qreal>(fila) * chunkHeight;

            c.area = QRectF(x0, y0, chunkWidth, chunkHeight);

            // Centro geométrico del chunk
            c.centro = Vector2D(
                x0 + chunkWidth  / 2.0,
                y0 + chunkHeight / 2.0
                );

            c.obstaculos.clear();
            c.debugRect = nullptr;

            // ----- DEBUG VISUAL -----
            if (fondoScroll) {
                auto *rectItem = new QGraphicsRectItem(c.area, fondoScroll);
                QPen pen(Qt::black);
                pen.setWidth(1);
                pen.setCosmetic(true); // grosor constante sin importar zoom
                rectItem->setPen(pen);
                rectItem->setBrush(Qt::NoBrush);
                rectItem->setZValue(-500);   // encima del fondo, por debajo de casi todo

                // visible sólo si el flag está activo
                rectItem->setVisible(debugChunks);

                c.debugRect = rectItem;
            }

            chunks.push_back(c);
        }
    }

    qDebug() << "Chunks creados:" << chunks.size()
             << " | filas:" << numFilasChunks
             << " columnas:" << numColsChunks;
}

Obstaculo* Nivel::crearObstaculoEnChunk(Chunk &chunk,
                                        qreal refHalfSize,
                                        qreal xRef,
                                        qreal yRef,
                                        int cuadrante,
                                        const QString &spriteName)
{
    if (!fondoScroll)
        return nullptr;

    // Mitad real del chunk (puede variar en la última fila)
    const qreal halfRealX = chunk.area.width()  / 2.0;
    const qreal halfRealY = chunk.area.height() / 2.0;

    // Escalas para pasar de sistema de referencia (refHalfSize)
    // al tamaño real del chunk.
    const qreal scaleX = halfRealX / refHalfSize;
    const qreal scaleY = halfRealY / refHalfSize;

    qreal dx = xRef * scaleX;
    qreal dy = yRef * scaleY;

    // Signos según cuadrante (YA ADAPTADOS, NO TOCAR)
    qreal sx = 0.0;
    qreal sy = 0.0;
    switch (cuadrante) {
    case 1:  sx = -1.0; sy =  1.0; break;
    case 2:  sx = -1.0; sy = -1.0; break;
    case 3:  sx =  1.0; sy = -1.0; break;
    case 4:  sx =  1.0; sy =  1.0; break;
    default: sx =  1.0; sy =  1.0; break;
    }

    Vector2D posLocal = chunk.centro + Vector2D(sx * dx, sy * dy);

    // Ruta completa del sprite
    const QString spritePath =
        QStringLiteral(":obs/nivel_3/%1.png").arg(spriteName);

    // Por ahora: hitbox circular genérico, solo cambia el sprite
    Obstaculo *obs = new Obstaculo(30.0, spritePath);

    obs->setParentItem(fondoScroll);
    obs->setPos(posLocal.x(), posLocal.y());

    // Registrar en el nivel y en el chunk
    obstaculos.push_back(obs);
    chunk.obstaculos.push_back(obs);

    return obs;
}

void Nivel::colocarObstaculosPatron1(Chunk &chunk)
{
    const qreal refHalfSize = 9.0; // el "9" de |1-9

    // Bloque I
    crearObstaculoEnChunk(chunk, refHalfSize, 1.5, 4.5, 1, "Piedra_2");
    crearObstaculoEnChunk(chunk, refHalfSize, 0.0, 1.0, 1, "circulo_4");
    crearObstaculoEnChunk(chunk, refHalfSize, 0.0, 1.5, 1, "arbol_hojas_2");
    crearObstaculoEnChunk(chunk, refHalfSize, 4.5, 1.0, 1, "arbol_hojas_3");

    // Bloque II
    crearObstaculoEnChunk(chunk, refHalfSize, 5.0, 8.0, 2, "Piedra_3");
    crearObstaculoEnChunk(chunk, refHalfSize, 8.0, 6.0, 2, "arbol_tronco_3");
    crearObstaculoEnChunk(chunk, refHalfSize, 7.5, 4.0, 2, "caja_1_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 5.0, 3.5, 2, "caja_3_1");

    // Bloque III
    crearObstaculoEnChunk(chunk, refHalfSize, 3.0, 8.5, 3, "Piedra_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 4.5, 3.5, 3, "circulo_3");
    crearObstaculoEnChunk(chunk, refHalfSize, 4.5, 5.0, 3, "arbol_hojas_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 0.5, 4.5, 3, "caja_3");
    crearObstaculoEnChunk(chunk, refHalfSize, 5.5, 6.0, 3, "circulo_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 2.0, 6.0, 3, "circulo_2");
}

void Nivel::colocarObstaculosPatron2(Chunk &chunk)
{
    const qreal refHalfSize = 10.5; // el "10.5" de |2-10.5

    // Primer bloque
    crearObstaculoEnChunk(chunk, refHalfSize, 4.0,  9.0, 2, "arbol_hojas_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 6.0,  7.0, 2, "caja_3_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 2.5,  3.0, 2, "caja_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 1.0,  7.0, 1, "caja_3");
    crearObstaculoEnChunk(chunk, refHalfSize, 3.5,  6.0, 1, "circulo_4");

    // Segundo bloque
    crearObstaculoEnChunk(chunk, refHalfSize, 9.2,  0.0, 2, "Piedra_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 6.2,  3.0, 3, "Piedra_2");

    // Tercer bloque
    crearObstaculoEnChunk(chunk, refHalfSize, 3.5,  4.0, 4, "Piedra_3");
    crearObstaculoEnChunk(chunk, refHalfSize, 2.0,  4.0, 4, "caja_2");
    crearObstaculoEnChunk(chunk, refHalfSize, 1.5,  6.0, 4, "caja_1_2");
}

void Nivel::colocarObstaculosPatron3(Chunk &chunk)
{
    const qreal refHalfSize = 8.5; // el "8.5" de |3-8.5

    // Primer bloque
    crearObstaculoEnChunk(chunk, refHalfSize, 2.5, 7.0, 2, "Piedra_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 2.5, 5.0, 2, "circulo_4");
    crearObstaculoEnChunk(chunk, refHalfSize, 1.0, 5.0, 2, "caja_3_2");
    crearObstaculoEnChunk(chunk, refHalfSize, 2.0, 6.5, 1, "arma");

    // Segundo bloque
    crearObstaculoEnChunk(chunk, refHalfSize, 6.5, 1.5, 2, "Piedra_3");
    crearObstaculoEnChunk(chunk, refHalfSize, 8.5, 1.0, 2, "caja_1");

    // Tercer bloque
    crearObstaculoEnChunk(chunk, refHalfSize, 1.0, 1.0, 3, "circulo_1");
    crearObstaculoEnChunk(chunk, refHalfSize, 2.0, 2.0, 3, "caja_1_2");
    crearObstaculoEnChunk(chunk, refHalfSize, 0.5, 1.5, 2, "arbol_hojas_4");
    crearObstaculoEnChunk(chunk, refHalfSize, 2.0, 3.0, 3, "Piedra_2");
}

void Nivel::poblarObstaculosPatronCiclico()
{
    if (chunks.empty())
        return;

    // Recorremos fila por fila
    for (int fila = 0; fila < numFilasChunks; ++fila) {

        // Tipo inicial de esta fila:
        // fila 0 -> 1
        // fila 1 -> 2
        // fila 2 -> 3
        // fila 3 -> 1
        int tipo = (fila % 3) + 1;

        for (int col = 0; col < numColsChunks; ++col) {

            const int idx = fila * numColsChunks + col;
            if (idx < 0 || idx >= static_cast<int>(chunks.size()))
                continue;   // seguridad

            Chunk &c = chunks[idx];

            switch (tipo) {
            case 1:
                colocarObstaculosPatron1(c);
                break;
            case 2:
                colocarObstaculosPatron2(c);
                break;
            case 3:
            default:
                colocarObstaculosPatron3(c);
                break;
            }

            // Avanzar tipo 1 -> 2 -> 3 -> 1 -> ...
            ++tipo;
            if (tipo > 3)
                tipo = 1;
        }
    }
}

void Nivel::inicializarEscena()
{
    // Color de fondo por nivel
    switch (numNivel) {
    case 1: escena->setBackgroundBrush(QColor(135,206,235)); break;
    case 2: escena->setBackgroundBrush(QColor(25,25,112));   break;
    case 3: /* usa imagen */                                  break;
    }
}

void Nivel::cargarElementosNivel()
{
    // Jugador
    jugador = new Cadete(15, 0.0, 0.0, true, 1000);
    jugador->setPos(0, 0);
    escena->addItem(jugador);

    // Munición inicial del jugador
    jugador->definirMunicion(30, 30);

    // velocidad del jugador usando la constante
    jugador->setVelocidad(VELOCIDAD_JUGADOR);

    // Oleadas iniciales
    actualizarOleadas();

}

// ============================
// 3) Loop principal del juego
// ============================

void Nivel::actualizarJuego()
{
    if (juegoTerminado)
        return;

    // Mover cámara
    if (m_moveLeft || m_moveRight || m_moveUp || m_moveDown)
        actualizarFondo();

    // IA / oleadas
    actualizarIA();
    actualizarOleadas();

    // Proyectiles
    avanzarProyectiles();
    limpiarProyectilesMuertos();

    // Enemigos muertos
    desactivarEnemigosMuertos();

    // Recarga jugador
    actualizarRecargaJugador();

    // HUD / colisiones
    actualizarHUD();
    manejarColisiones();

    // --- Game Over por muerte del jugador ---
    if (jugador && jugador->estaMuerto()) {
        finJuegoPorMuerte();
    }
}

// --- Subrutinas del loop ---

void Nivel::actualizarFondo()
{
    // Desplazamiento base
    const qreal speed = 3.0;
    Vector2D dir(0,0);

    if (m_moveUp)    dir.setY(dir.y() + 1);
    if (m_moveDown)  dir.setY(dir.y() - 1);
    if (m_moveLeft)  dir.setX(dir.x() + 1);
    if (m_moveRight) dir.setX(dir.x() - 1);
    if (dir.magnitud2() == 0) return;

    dir = dir.normalizado();

    // Guardar prev
    Vector2D camaraAnterior   = camara;
    QPointF  posFondoAnterior = fondoScroll->pos();

    // Aplicar movimiento
    Vector2D nuevaCamara = camara - (dir * speed);

    // Limitar cámara
    if (nuevaCamara.x() < 0) nuevaCamara.setX(0);
    if (nuevaCamara.x() > limiteMapa.x()) nuevaCamara.setX(limiteMapa.x());
    if (nuevaCamara.y() < 0) nuevaCamara.setY(0);
    if (nuevaCamara.y() > limiteMapa.y()) nuevaCamara.setY(limiteMapa.y());

    camara = nuevaCamara;
    actualizarPosicionFondo();

    // Evitar atravesar obstáculo
    if (jugadorTocaObstaculo()) {
        camara = camaraAnterior;
        fondoScroll->setPos(posFondoAnterior);
    }
}

void Nivel::actualizarPosicionFondo()
{
    // Reposicionar fondo por cámara
    Vector2D origenFondo = (viewportSize / -2.0) - camara;
    fondoScroll->setPos(origenFondo.x(), origenFondo.y());
}

void Nivel::actualizarIA()
{
    //Actualizar agentes activos
    for(auto *a : agentes)
        if(a && a->estaActivo())
            a->actualizar();
}

void Nivel::actualizarOleadas()
{
    // Detectar rondas existentes
    const bool hayRonda1 = existeRonda(1);
    const bool hayRonda2 = existeRonda(2);
    const bool hayRonda3 = existeRonda(3);

    // Crear ronda 1
    if (!hayRonda1 && ronda_act == 1) {
        crearRonda1();
        return;
    }

    // Crear ronda 2
    if (!hayRonda2 && ronda_act == 2) {
        crearRonda2();
        return;
    }

    // Crear ronda 3 (Rotación)
    if (!hayRonda3 && ronda_act == 3) {
        crearRonda3();
        return;
    }

    // Activar grupos de la ronda actual
    activarGruposRondaActual();

    // Avanzar de ronda si se completó
    avanzarRondaSiCompleta();

    // Sinergia emboscada en ronda 2
    if (ronda_act == 2) sinergiaEmboscadaRonda2();

    // Coordinación especial de Rotación en la ronda 3
    if (ronda_act == 3) {
        coordinarRotacionRonda3();
    }
}

void Nivel::avanzarProyectiles()
{
    // Avanzar todos
    for (auto *p : proyectiles)
        if (p) p->avanzar();
}

void Nivel::limpiarProyectilesMuertos()
{
    // Limpiar proyectiles muertos
    proyectiles.erase(
        std::remove_if(
            proyectiles.begin(),
            proyectiles.end(),
            [&](Proyectil* p){
                if (p && p->muerto) {
                    escena->removeItem(p);
                    delete p;
                    return true;
                }
                return false;
            }),
        proyectiles.end()
        );
}

void Nivel::desactivarEnemigosMuertos()
{
    for (FuerzaArmada *p : enemigos) {
        if (!p) continue;
        if (p->estaMuerto() && p->scene() != nullptr) {
            escena->removeItem(p);
            p->setEnabled(false);
            p->setVisible(false);
        }
    }
}

void Nivel::actualizarHUD()
{
    // -------- VIDA --------
    static int vidaMaxima = -1;

    if (!jugador) {
        barraVida->setRange(0, 1);
        barraVida->setValue(0);
        barraVida->setFormat("Muerto");
    } else {
        int vidaActual = jugador->getVida();

        if (vidaMaxima < 0) {
            vidaMaxima = (vidaActual > 0) ? vidaActual : 1;
            barraVida->setRange(0, vidaMaxima);
        }

        int valor = vidaActual;
        if (valor < 0) valor = 0;
        if (valor > vidaMaxima) valor = vidaMaxima;

        barraVida->setValue(valor);
        barraVida->setFormat(
            QString("%1 / %2 HP").arg(vidaActual).arg(vidaMaxima)
            );
    }

    // -------- ENEMIGOS (muertos / totales) --------
    int totalEnemigos   = 0;
    int enemigosMuertos = 0;

    for (FuerzaArmada *e : enemigos) {
        if (!e) continue;
        ++totalEnemigos;
        if (e->estaMuerto())
            ++enemigosMuertos;
    }

    if (totalEnemigos == 0) {
        lblEnemigos->setText("-- / --");
    } else {
        lblEnemigos->setText(
            QString("%1 / %2")
                .arg(enemigosMuertos)
                .arg(totalEnemigos)
            );
    }

    // -------- RONDAS (máx 3) --------
    if (ronda_act <= total_rondas) {
        lblRonda->setText(
            QString("%1 / %2")
                .arg(ronda_act)
                .arg(total_rondas)
            );
    } else {
        // Ya no hay más rondas definidas
        lblRonda->setText("Completado");
    }

    // -------- MUNICIÓN --------
    if (jugador) {
        // Asumiendo que Cadete tiene estos getters (ya los habías dejado en el comentario)
        lblBalas->setText(
            QString("%1 / %2")
                .arg(jugador->getBalas())
                .arg(jugador->getBalasMax())
            );
    } else {
        lblBalas->setText("-- / --");
    }
}

// ============================
// 4) Entradas (mouse/teclado)
// ============================

void Nivel::mouseMoveEvent(QMouseEvent *event)
{
    if (!jugador) return;

    // Dirección hacia el mouse
    QPointF posEscena  = vista->mapToScene(event->pos());
    QPointF posJugador = jugador->scenePos();
    Vector2D dirMouse(posEscena.x() - posJugador.x(),
                      posEscena.y() - posJugador.y());

    if (dirMouse.magnitud2() > 0) {
        mouseDir = dirMouse.normalizado();
        jugador->setDireccion(mouseDir);
        jugador->update();
    }
}

void Nivel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {

        // Si está recargando, el click cancela la recarga y NO dispara
        if (recargandoJugador) {
            cancelarRecargaJugador();
            return;
        }

        // Si no está recargando: disparo normal
        disparar(jugador);
    }
}

bool Nivel::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == vista->viewport()) {
        // Bloquear rueda
        if (event->type() == QEvent::Wheel) return true;

        // Pasar movimiento a handler
        if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent*>(event);
            mouseMoveEvent(me);
            return false;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void Nivel::keyReleaseEvent(QKeyEvent *event)
{
    // Soltar flags
    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:  m_moveLeft  = false; break;
    case Qt::Key_D:
    case Qt::Key_Right: m_moveRight = false; break;
    case Qt::Key_W:
    case Qt::Key_Up:    m_moveUp    = false; break;
    case Qt::Key_S:
    case Qt::Key_Down:  m_moveDown  = false; break;
    }
}

void Nivel::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:  m_moveLeft  = true; break;
    case Qt::Key_D:
    case Qt::Key_Right: m_moveRight = true; break;
    case Qt::Key_W:
    case Qt::Key_Up:    m_moveUp    = true; break;
    case Qt::Key_S:
    case Qt::Key_Down:  m_moveDown  = true; break;

    case Qt::Key_R:
        iniciarRecargaJugador();
        break;

    // Mostrar / ocultar grilla de chunks
    case Qt::Key_F3:
        debugChunks = !debugChunks;
        actualizarDebugChunksVisibles();
        break;

    // Reset del zoom al valor por defecto
    case Qt::Key_F6:
        zoomActual = zoomDefault;
        aplicarZoomVista();
        break;

    // Zoom out (alejar)
    case Qt::Key_F7: {
        qreal nuevo = zoomActual / zoomStep;
        if (nuevo < zoomMin) nuevo = zoomMin;

        if (!qFuzzyCompare(nuevo, zoomActual)) {
            zoomActual = nuevo;
            aplicarZoomVista();
        } else {
            qDebug() << "[ZOOM] min alcanzado:" << zoomActual << "x";
        }
        break;
    }

    // Zoom in (acercar)
    case Qt::Key_F8: {
        qreal nuevo = zoomActual * zoomStep;
        if (nuevo > zoomMax) nuevo = zoomMax;

        if (!qFuzzyCompare(nuevo, zoomActual)) {
            zoomActual = nuevo;
            aplicarZoomVista();
        } else {
            qDebug() << "[ZOOM] max alcanzado:" << zoomActual << "x";
        }
        break;
    }
    }
}

// ============================
// 5) Acciones de juego sueltas
// ============================

void Nivel::disparar(Cadete *emisor)
{
    if (!emisor) return;
    if (!emisor->esJugador() && emisor->estaMuerto()) return;

    // Consumir bala antes de crear el proyectil.
    // Si no tenía munición, no se dispara nada.
    if (!emisor->consumirBala())
        return;

    // Pos en fondo
    QPointF posEnFondo = fondoScroll->mapFromScene(emisor->scenePos());

    // Crear bala
    Bala *b = new Bala(emisor, emisor->getDireccion());
    b->setParentItem(fondoScroll);
    b->setPos(posEnFondo);

    // Guardar
    proyectiles.push_back(b);
}

bool Nivel::jugadorTocaObstaculo() const
{
    if (!jugador) return false;

    // Chequear colisión con obstáculos
    QList<QGraphicsItem*> cols = jugador->collidingItems();
    for (QGraphicsItem *item : cols)
        if (dynamic_cast<Obstaculo*>(item))
            return true;

    return false;
}


void Nivel::manejarColisiones() {}
void Nivel::onVolverClicked()
{
    // Volver al menú
    if (timer) timer->stop();
    if (m_sonidoAmbiente) {
        m_sonidoAmbiente->stop();
    }
    emit volverAlMenu();
}

// ============================
// 6) Helpers
// ============================

void Nivel::crearObstaculosFijos()
{
    // Centro del fondo
    auto fondoCentro = Vector2D(fondoSize.x() / 2.0, fondoSize.y() / 2.0);

    // Piedra
    Obstaculo *piedra = new Obstaculo(30);
    piedra->setParentItem(fondoScroll);
    piedra->setPos(fondoCentro.x() - 200, fondoCentro.y() - 50);
    obstaculos.push_back(piedra);

    // Caja
    Obstaculo *caja = new Obstaculo(60, 40);
    caja->setParentItem(fondoScroll);
    caja->setPos(fondoCentro.x() + 150, fondoCentro.y() - 80);
    obstaculos.push_back(caja);

    // Bulto
    Obstaculo *bulto = new Obstaculo(20);
    bulto->setParentItem(fondoScroll);
    bulto->setPos(fondoCentro.x() - 120, fondoCentro.y() + 120);
    obstaculos.push_back(bulto);

    // Pared
    Obstaculo *pared = new Obstaculo(120, 20);
    pared->setParentItem(fondoScroll);
    pared->setPos(fondoCentro.x() / 2.0 + 250, fondoCentro.y() + 150);
    obstaculos.push_back(pared);
}

bool Nivel::existeRonda(int r) const
{
    // ¿Hay agentes de la ronda r?
    for (Agente *a : agentes) {
        if (!a) continue;
        if (a->getRondaAsignada() == r) return true;
    }
    return false;
}

void Nivel::crearRonda1()
{
    // Dos grupos ataque directo
    auto *g1 = new OleadaCadetes(this);
    g1->setModo(ModoGrupo::AtaqueDirecto);
    g1->setEstado(EstadoGrupo::Atacando);
    g1->setRondaAsignada(1);
    registrarAgente(g1);
    g1->spawnRonda(5, 300.0, 500.0, -M_PI * 1.7, -M_PI * 1.5);

    auto *g2 = new OleadaCadetes(this);
    g2->setModo(ModoGrupo::AtaqueDirecto);
    g2->setEstado(EstadoGrupo::Atacando);
    g2->setRondaAsignada(1);
    registrarAgente(g2);
    g2->spawnRonda(5, 300.0, 500.0, -M_PI * 1.3, -M_PI * 1.1);
}

void Nivel::crearRonda2()
{
    // Necesitamos el fondo para poder convertir entre coords locales y de escena
    auto *fondo = getFondoScroll();
    if (!fondo) return;

    // Centro del fondo en coordenadas LOCALES del fondo
    Vector2D fondoCentro(fondoSize.x() / 2.0,
                         fondoSize.y() / 2.0);

    // ==============================
    //  Slots flanqueo EN LOCAL FONDO
    // ==============================

    // Izquierda (local del fondo)
    std::vector<Vector2D> slotsIzqLocal = {
        fondoCentro + Vector2D(-350.0, -150.0),
        fondoCentro + Vector2D(-350.0,  -50.0),
        fondoCentro + Vector2D(-350.0,   50.0),
        fondoCentro + Vector2D(-350.0,  150.0)
    };

    // Derecha (local del fondo)
    std::vector<Vector2D> slotsDerLocal = {
        fondoCentro + Vector2D( 350.0, -150.0),
        fondoCentro + Vector2D( 350.0,  -50.0),
        fondoCentro + Vector2D( 350.0,   50.0),
        fondoCentro + Vector2D( 350.0,  150.0)
    };

    const qreal radioAct = 220.0;

    // =======================
    //  Grupo izquierda (Campamento)
    // =======================
    auto *gIzq = new OleadaCadetes(this);
    gIzq->setModo(ModoGrupo::Campamento);
    gIzq->setEstado(EstadoGrupo::Preparando);
    gIzq->setRondaAsignada(2);
    gIzq->setPuntosCampamento(slotsIzqLocal);
    gIzq->setRadioActivacion(radioAct);
    registrarAgente(gIzq);
    gIzq->spawnRonda(6,
                     300.0, 500.0,
                     -M_PI * 1.2, -M_PI * 0.8);

    // =======================
    //  Grupo derecha (Campamento)
    // =======================
    auto *gDer = new OleadaCadetes(this);
    gDer->setModo(ModoGrupo::Campamento);
    gDer->setEstado(EstadoGrupo::Preparando);
    gDer->setRondaAsignada(2);
    gDer->setPuntosCampamento(slotsDerLocal);
    gDer->setRadioActivacion(radioAct);
    registrarAgente(gDer);
    gDer->spawnRonda(6,
                     300.0, 500.0,
                     -M_PI * 0.2, 0.0);
}


void Nivel::crearRonda3()
{
    auto *fondo = getFondoScroll();
    if (!fondo) return;

    // Centro del fondo en coordenadas LOCALES del fondo
    Vector2D fondoCentro(fondoSize.x() / 2.0,
                         fondoSize.y() / 2.0);

    // =====================================================
    //  SLOTS EN LOCAL-FONDO
    // =====================================================

    // Grupo frontal (por arriba del jugador)
    std::vector<Vector2D> slotsCentroLocal = {
        fondoCentro + Vector2D(-60.0, -260.0),
        fondoCentro + Vector2D(  0.0, -280.0),
        fondoCentro + Vector2D( 60.0, -260.0),
        fondoCentro + Vector2D(-30.0, -230.0),
        fondoCentro + Vector2D( 30.0, -230.0)
    };

    // Grupo lateral izquierdo
    std::vector<Vector2D> slotsIzqLocal = {
        fondoCentro + Vector2D(-320.0, -60.0),
        fondoCentro + Vector2D(-360.0,   0.0),
        fondoCentro + Vector2D(-320.0,  60.0),
        fondoCentro + Vector2D(-280.0,   0.0),
        fondoCentro + Vector2D(-300.0, -40.0)
    };

    // Grupo lateral derecho
    std::vector<Vector2D> slotsDerLocal = {
        fondoCentro + Vector2D(320.0, -60.0),
        fondoCentro + Vector2D(360.0,   0.0),
        fondoCentro + Vector2D(320.0,  60.0),
        fondoCentro + Vector2D(280.0,   0.0),
        fondoCentro + Vector2D(300.0, -40.0)
    };

    // =====================================================
    //  PUNTOS DE RETIRADA EN LOCAL-FONDO -> ESCENA
    // =====================================================

    // Retirada frontal (más arriba)
    Vector2D retiroCentroLocal = fondoCentro + Vector2D(0.0, -550.0);

    // Retirada izquierda
    Vector2D retiroIzqLocal = fondoCentro + Vector2D(-550.0, 0.0);

    // Retirada derecha
    Vector2D retiroDerLocal = fondoCentro + Vector2D(550.0, 0.0);

    // =====================================================
    //  Creación de grupos Rotación (igual que antes, pero
    //  usando slots*Scene y retiro*Scene)
    // =====================================================

    const int   cantPorGrupo = 6;
    const qreal radioMin     = 300.0;
    const qreal radioMax     = 520.0;
    const qreal radioAct     = 260.0;

    // --- Grupo frontal ---
    auto *gCentro = new OleadaCadetes(this);
    gCentro->setModo(ModoGrupo::Rotacion);
    gCentro->setEstado(EstadoGrupo::Preparando);
    gCentro->setRondaAsignada(3);
    gCentro->setPuntosCampamento(slotsCentroLocal);
    gCentro->setPuntoRetirada(retiroCentroLocal);
    gCentro->setRadioActivacion(radioAct);
    registrarAgente(gCentro);
    gCentro->spawnRonda(cantPorGrupo,
                        radioMin, radioMax,
                        -M_PI * 0.8, -M_PI * 0.2);

    // --- Grupo izquierdo ---
    auto *gIzq = new OleadaCadetes(this);
    gIzq->setModo(ModoGrupo::Rotacion);
    gIzq->setEstado(EstadoGrupo::Preparando);
    gIzq->setRondaAsignada(3);
    gIzq->setPuntosCampamento(slotsIzqLocal);
    gIzq->setPuntoRetirada(retiroIzqLocal);
    gIzq->setRadioActivacion(radioAct);
    registrarAgente(gIzq);
    gIzq->spawnRonda(cantPorGrupo,
                     radioMin, radioMax,
                     M_PI * 0.8, M_PI * 1.2);

    // --- Grupo derecho ---
    auto *gDer = new OleadaCadetes(this);
    gDer->setModo(ModoGrupo::Rotacion);
    gDer->setEstado(EstadoGrupo::Preparando);
    gDer->setRondaAsignada(3);
    gDer->setPuntosCampamento(slotsDerLocal);
    gDer->setPuntoRetirada(retiroDerLocal);
    gDer->setRadioActivacion(radioAct);
    registrarAgente(gDer);
    gDer->spawnRonda(cantPorGrupo,
                     radioMin, radioMax,
                     -M_PI * 0.2, M_PI * 0.2);
}


void Nivel::activarGruposRondaActual()
{
    // Activar grupos de la ronda actual con enemigos
    for (Agente *a : agentes) {
        if (!a) continue;
        if (a->getRondaAsignada() != ronda_act) continue;
        if (a->estaActivo())                    continue;
        if (a->getEnemigosRestantes() <= 0)     continue;
        a->setActivo(true);
    }
}

void Nivel::avanzarRondaSiCompleta()
{
    // Contar terminados
    int gruposEnRonda      = 0;
    int gruposTerminados   = 0;

    for (Agente *a : agentes) {
        if (!a) continue;
        if (a->getRondaAsignada() != ronda_act) continue;

        gruposEnRonda++;
        if (a->rondaCompletada())
            gruposTerminados++;
    }

    // Pasar de ronda
    if (gruposEnRonda > 0 && gruposTerminados == gruposEnRonda) {
        ronda_act++;

        // Si ya pasamos la última ronda -> victoria
        if (ronda_act > total_rondas) {
            finJuegoPorVictoria();
        }
        // Aquí podrías meter feedback tipo "Ronda superada" si quieres
    }
}

void Nivel::sinergiaEmboscadaRonda2()
{
    // ¿Algún flanqueo ya ataca?
    bool hayFlanqueoAtacando = false;

    for (Agente *a : agentes) {
        if (!a) continue;
        if (a->getRondaAsignada() != 2)             continue;
        if (!a->estaActivo())                        continue;
        if (a->getModo() != ModoGrupo::AtaqueDirecto)     continue;
        if (a->getEnemigosRestantes() <= 0)          continue;
        if (a->getEstado() == EstadoGrupo::Atacando) { qDebug() << " estan atacando en ronda 2"; hayFlanqueoAtacando = true; break; }
    }

    // Poner modo Emboscada a todos los de ronda 2
    if (hayFlanqueoAtacando) {
        for (Agente *a : agentes) {
            if (!a) continue;
            if (a->getRondaAsignada() != 2)    continue;
            if (a->getEnemigosRestantes() <= 0) continue;
            a->setModo(ModoGrupo::AtaqueDirecto);
            a->setEstado(EstadoGrupo::Atacando);
            a->setActivo(true);
        }
    }
}

void Nivel::coordinarRotacionRonda3()
{
    // Recorremos todas las oleadas de Rotación en ronda 3
    for (Agente *a : agentes) {
        if (!a) continue;
        if (a->getRondaAsignada() != 3) continue;
        if (a->getModo() != ModoGrupo::Rotacion) continue;
        if (a->getEnemigosRestantes() <= 0) continue;

        auto *petidora = dynamic_cast<OleadaCadetes*>(a);
        if (!petidora) continue;

        // Solo nos interesan las oleadas que están huyendo
        if (!petidora->estaHuyendo())
            continue;

        // Buscar aliado más cercano que NO esté huyendo
        OleadaCadetes *aliado = encontrarAliadoMasCercanoEnRotacion(petidora);
        if (!aliado)
            continue;

        // Marcar a ese aliado para que pase a Atacando en su lógica interna
        aliado->marcarLlamadoPorOtraOleada();
    }
}

// sistema de sonido
void Nivel::cargarSonidos()
{
    cargarSonidosDesdeArchivos();
}

void Nivel::cargarSonidosDesdeArchivos()
{
    QString rutaBase = QCoreApplication::applicationDirPath();

    // Buscar carpeta de assets en rutas posibles
    QStringList posiblesRutas = {
        rutaBase + "/../../../assets/audio",
    };

    QString rutaEncontrada;
    for (const QString &ruta : posiblesRutas) {
        QString rutaLimpia = QDir::cleanPath(ruta);
        if (QDir(rutaLimpia).exists()) {
            QDir dir(rutaLimpia);
            QStringList archivos = dir.entryList(QDir::Files);
            if (!archivos.isEmpty()) {
                rutaEncontrada = rutaLimpia;
                break;
            }
        }
    }

    if (rutaEncontrada.isEmpty()) {
        return;
    }

    // Cargar sonido de ambiente de guerra
    QString rutaAmbiente = rutaEncontrada + "/Guerra-Efectos-de-sonido.wav";
    m_sonidoAmbiente = new QSoundEffect(this);

    if (QFile::exists(rutaAmbiente)) {
        m_sonidoAmbiente->setSource(QUrl::fromLocalFile(rutaAmbiente));
        m_sonidoAmbiente->setVolume(0.1f);
        m_sonidoAmbiente->setLoopCount(1);  // Solo una reproducción por vez

        // Conectar señal para reproducir en loop cuando termine
        connect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                this, &Nivel::onSonidoAmbienteTerminado);

        // Iniciar reproducción
        m_sonidoAmbiente->play();

    } else {
        delete m_sonidoAmbiente;
        m_sonidoAmbiente = nullptr;
    }
}

void Nivel::onSonidoAmbienteTerminado()
{
    // Este slot se llama cada vez que cambia el estado de reproducción
    if (m_sonidoAmbiente && !m_sonidoAmbiente->isPlaying()) {
        // Si el sonido terminó de reproducirse, reproducirlo nuevamente
        // (esto crea un loop continuo)
        m_sonidoAmbiente->play();
    }
}


OleadaCadetes* Nivel::encontrarAliadoMasCercanoEnRotacion(OleadaCadetes *petidora)
{
    if (!petidora) return nullptr;

    OleadaCadetes *mejor = nullptr;
    qreal mejorDist2 = std::numeric_limits<qreal>::max();

    Vector2D centroPetidora = petidora->centroGrupoScene();

    for (Agente *a : agentes) {
        if (!a) continue;

        auto *g = dynamic_cast<OleadaCadetes*>(a);
        if (!g) continue;
        if (g == petidora) continue;
        if (g->getRondaAsignada() != 3) continue;
        if (g->getModo() != ModoGrupo::Rotacion) continue;
        if (g->getEnemigosRestantes() <= 0) continue;

        // No queremos que el aliado también esté huyendo
        if (g->estaHuyendo()) continue;

        Vector2D centroAliado = g->centroGrupoScene();
        qreal dist2 = (centroAliado - centroPetidora).magnitud2();

        if (dist2 < mejorDist2) {
            mejorDist2 = dist2;
            mejor      = g;
        }
    }

    return mejor;
}

void Nivel::iniciarRecargaJugador()
{
    if (!jugador) return;
    if (recargandoJugador) return;

    // Si ya está al máximo, no tiene sentido recargar
    if (jugador->getBalasMax() <= 0) return;
    if (jugador->getBalas() >= jugador->getBalasMax()) return;

    recargandoJugador   = true;
    recargaTicksActual  = 0;

    if (barraRecarga) {
        barraRecarga->setRange(0, recargaTicksTotal);
        barraRecarga->setValue(0);
        barraRecarga->setVisible(true);
    }
}

void Nivel::cancelarRecargaJugador()
{
    recargandoJugador  = false;
    recargaTicksActual = 0;

    if (barraRecarga) {
        barraRecarga->setVisible(false);
    }
}

void Nivel::actualizarRecargaJugador()
{
    if (!recargandoJugador)
        return;

    if (!jugador || jugador->estaMuerto()) {
        cancelarRecargaJugador();
        return;
    }

    // Avanzar el "tiempo" de recarga
    ++recargaTicksActual;

    if (barraRecarga) {
        barraRecarga->setValue(recargaTicksActual);
    }

    // ¿Terminó la recarga?
    if (recargaTicksActual >= recargaTicksTotal) {
        recargandoJugador  = false;
        recargaTicksActual = 0;

        if (barraRecarga) {
            barraRecarga->setVisible(false);
        }

        // Aquí se recargan realmente las balas del jugador
        jugador->recargar();
    }
}

void Nivel::finJuegoPorMuerte()
{
    if (juegoTerminado)
        return;

    juegoTerminado = true;

    if (timer)               timer->stop();
    if (timerDisparoEnemigos) timerDisparoEnemigos->stop();

    mostrarGameOverOverlay(false);
}

void Nivel::finJuegoPorVictoria()
{
    if (juegoTerminado)
        return;

    juegoTerminado = true;

    if (timer)               timer->stop();
    if (timerDisparoEnemigos) timerDisparoEnemigos->stop();

    mostrarGameOverOverlay(true);
}

void Nivel::mostrarGameOverOverlay(bool victoria)
{
    if (overlayGameOver) {
        overlayGameOver->raise();
        overlayGameOver->show();
        return;
    }

    overlayGameOver = new QWidget(this);
    overlayGameOver->setGeometry(0, 0, width(), height());
    overlayGameOver->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    overlayGameOver->setStyleSheet("background-color: rgba(0, 0, 0, 180);");

    auto *rootLayout = new QVBoxLayout(overlayGameOver);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *panel = new QWidget(overlayGameOver);
    panel->setObjectName("panelGameOver");
    panel->setStyleSheet(
        "#panelGameOver {"
        "  background-color: rgba(15, 15, 25, 230);"
        "  border-radius: 16px;"
        "  border: 1px solid rgba(255, 255, 255, 40);"
        "}"
        "QLabel {"
        "  color: #f0f0f0;"
        "}"
        "QPushButton {"
        "  background-color: rgba(60, 60, 90, 220);"
        "  color: #f0f0f0;"
        "  border-radius: 8px;"
        "  padding: 6px 18px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(90, 90, 130, 240);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(40, 40, 70, 240);"
        "}"
        );

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(32, 24, 32, 24);
    panelLayout->setSpacing(12);

    // --- Título ---
    lblGameOverTitulo = new QLabel(victoria ? "VICTORIA" : "GAME OVER", panel);
    lblGameOverTitulo->setAlignment(Qt::AlignCenter);
    lblGameOverTitulo->setStyleSheet(
        "font-size: 28px; font-weight: 800; letter-spacing: 4px;"
        );
    panelLayout->addWidget(lblGameOverTitulo);

    // --- Estadísticas ---
    lblGameOverStats = new QLabel(panel);
    lblGameOverStats->setAlignment(Qt::AlignCenter);
    lblGameOverStats->setStyleSheet("font-size: 13px; color: #d0d0d0;");

    // calcular stats
    int totalEnemigos   = 0;
    int enemigosMuertos = 0;

    for (FuerzaArmada *e : enemigos) {
        if (!e) continue;
        ++totalEnemigos;
        if (e->estaMuerto())
            ++enemigosMuertos;
    }

    int rondasCompletadas = ronda_act - 1;
    if (rondasCompletadas < 0) rondasCompletadas = 0;
    if (rondasCompletadas > total_rondas) rondasCompletadas = total_rondas;

    int vidaFinal = (jugador ? jugador->getVida() : 0);

    QString textoResumen = QString(
                               "Rondas completadas: %1 / %2\n"
                               "Enemigos eliminados: %3 / %4\n"
                               "Vida final: %5"
                               ).arg(rondasCompletadas)
                               .arg(total_rondas)
                               .arg(enemigosMuertos)
                               .arg(totalEnemigos)
                               .arg(vidaFinal);

    lblGameOverStats->setText(textoResumen);
    panelLayout->addWidget(lblGameOverStats);

    // --- Botones ---
    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(16);
    buttonsLayout->setAlignment(Qt::AlignCenter);

    btnReintentar   = new QPushButton("Reintentar", panel);
    btnMenuGameOver = new QPushButton("Volver al menú", panel);

    buttonsLayout->addWidget(btnReintentar);
    buttonsLayout->addWidget(btnMenuGameOver);

    panelLayout->addSpacing(8);
    panelLayout->addLayout(buttonsLayout);

    // Centrar panel en el overlay
    rootLayout->addStretch();
    rootLayout->addWidget(panel, 0, Qt::AlignCenter);
    rootLayout->addStretch();

    // Conexiones de los botones
    connect(btnReintentar, &QPushButton::clicked, this, [this]() {
        destruirOverlayGameOver();
        emit reintentarNivel(numNivel);
    });

    connect(btnMenuGameOver, &QPushButton::clicked, this, [this]() {
        destruirOverlayGameOver();
        emit volverAlMenu();
    });

    overlayGameOver->raise();
    overlayGameOver->show();
}

void Nivel::destruirOverlayGameOver()
{
    if (!overlayGameOver)
        return;

    overlayGameOver->deleteLater();
    overlayGameOver   = nullptr;
    lblGameOverTitulo = nullptr;
    lblGameOverStats  = nullptr;
    btnReintentar     = nullptr;
    btnMenuGameOver   = nullptr;
}

void Nivel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (hud) {
        hud->setGeometry(0, 0, width(), height());
    }
    if (overlayGameOver) {
        overlayGameOver->setGeometry(0, 0, width(), height());
    }
}

void Nivel::actualizarDebugChunksVisibles()
{
    for (Chunk &c : chunks) {
        if (c.debugRect) {
            c.debugRect->setVisible(debugChunks);
        }
    }
}

void Nivel::aplicarZoomVista()
{
    if (!vista) return;

    // Construimos la transformación desde cero (opción B)
    QTransform t;
    t.scale(zoomActual, zoomActual);
    vista->setTransform(t);

    // Centrar la vista en el jugador (si existe), que está en (0,0)
    if (jugador) {
        vista->centerOn(jugador);
    } else {
        vista->centerOn(0, 0);
    }

    // Debug muy breve a consola
    qDebug() << "[ZOOM]" << zoomActual << "x";
}
