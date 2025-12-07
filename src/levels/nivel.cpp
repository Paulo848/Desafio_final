#include "nivel.h"
#include "cadete.h"
#include "fuerzaarmada.h"
#include "obstaculo.h"

#include <QVBoxLayout>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QBrush>
#include <QPixmap>
#include <QDebug>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <algorithm>

#include "bala.h"
#include "oleadacadetes.h"

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
    jugador(nullptr),
    mouseDir(0.0, 0.0),
    m_moveLeft(false),
    m_moveRight(false),
    m_moveUp(false),
    m_moveDown(false),
    ronda_act(1),
    total_rondas(4)
{
    // Setup UI/escena/nivel
    inicializarUI();
    inicializarEscena();
    cargarElementosNivel();

    // Timer principal
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Nivel::actualizarJuego);
    timer->start(16);
}

Nivel::~Nivel()
{
    if (timer) timer->stop();

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

    // Rectángulo de escena
    escena->setSceneRect(-viewportSize.x()/2.0,
                         -viewportSize.y()/2.0,
                         viewportSize.x(),
                         viewportSize.y());

    // Fondo del nivel
    QPixmap imagenFondo(":/ui/ui/Fondo_Nivel_3.png");
    fondoSize.set(imagenFondo.width(), imagenFondo.height());
    limiteMapa = fondoSize - viewportSize;
    if (limiteMapa.x() < 0) limiteMapa.setX(0);
    if (limiteMapa.y() < 0) limiteMapa.setY(0);
    camara = limiteMapa / 2.0;

    fondoScroll = new QGraphicsPixmapItem(imagenFondo);
    fondoScroll->setZValue(-1000);
    actualizarPosicionFondo();
    escena->addItem(fondoScroll);

    // Config vista
    vista->setScene(escena);
    vista->setFocusPolicy(Qt::NoFocus);
    vista->setMouseTracking(true);
    vista->viewport()->setMouseTracking(true);
    vista->viewport()->installEventFilter(this);
    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setMouseTracking(true);

    // HUD simple
    hud = new QWidget(this);
    hud->setAttribute(Qt::WA_TransparentForMouseEvents);
    hud->setStyleSheet("background: transparent;");
    hud->setGeometry(0, 0, viewportSize.x(), viewportSize.y());

    barraVida = new QProgressBar(hud);
    barraVida->setGeometry(20, 20, 200, 25);
    barraVida->setRange(0, 100);
    barraVida->setValue(100);

    lblEnemigos = new QLabel("0 / 0", hud);
    lblEnemigos->setGeometry(20, 60, 150, 20);

    lblRonda = new QLabel("1 / 5", hud);
    lblRonda->setGeometry(20, 90, 150, 20);

    lblBalas = new QLabel("0 / 30", hud);
    lblBalas->setGeometry(20, 120, 150, 20);

    // Botón volver
    btnVolver = new QPushButton("Volver al Menú", this);
    btnVolver->setGeometry(20, viewportSize.y() - 50, 150, 30);
    connect(btnVolver, &QPushButton::clicked, this, &Nivel::onVolverClicked);

    // Final UI
    vista->centerOn(0,0);
    layout->addWidget(vista);
    setLayout(layout);
    setFocusPolicy(Qt::StrongFocus);
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

    // Oleadas iniciales
    actualizarOleadas();

    // Timer disparos enemigos
    timerDisparoEnemigos = new QTimer(this);
    connect(timerDisparoEnemigos, &QTimer::timeout, this, &Nivel::disparosEnemigos);
    timerDisparoEnemigos->start(500);

    // Obstáculos
    //crearObstaculosFijos();
    crearObstaculosAleatorios(/*num*/14, /*rmin*/80.0, /*rmax*/400.0);
}

// ============================
// 3) Loop principal del juego
// ============================

void Nivel::actualizarJuego()
{
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

    // HUD / colisiones
    actualizarHUD();
    manejarColisiones();
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
    // Actualizar agentes activos
    for (auto *a : agentes)
        if (a && a->estaActivo())
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
    // Vida
    barraVida->setValue(jugador ? jugador->getVida() : 0);

    // Enemigos (placeholder)
    lblEnemigos->setText(QString("%1 / %2")
                             .arg("")  // get enemigos matados
                             .arg("")); // get enemigos totales

    // Ronda
    lblRonda->setText(QString("%1 / %2").arg(ronda_act).arg(total_rondas));

    // Balas (placeholder)
    lblBalas->setText(QString("%1 / %2")
                          .arg("")   // jugador->getBalas()
                          .arg("")); // jugador->getBalasMax()
}

void Nivel::manejarColisiones()
{
    // Aquí va la lógica de colisiones
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
    // Disparo con click izquierdo
    if (event->button() == Qt::LeftButton)
        disparar(jugador);
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

void Nivel::keyPressEvent(QKeyEvent *event)
{
    // Flags de movimiento
    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:  m_moveLeft  = true; break;
    case Qt::Key_D:
    case Qt::Key_Right: m_moveRight = true; break;
    case Qt::Key_W:
    case Qt::Key_Up:    m_moveUp    = true; break;
    case Qt::Key_S:
    case Qt::Key_Down:  m_moveDown  = true; break;
    }
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

// ============================
// 5) Acciones de juego sueltas
// ============================

void Nivel::disparosEnemigos()
{
    for (auto *e : enemigos) {

        if (!e || e->estaMuerto() || !e->estaDisparando() || !e->tieneMunicion() )
            continue;
        disparar(e);
    }
}

void Nivel::disparar(Cadete *emisor)
{
    if (!emisor) return;
    if (!emisor->esJugador() && emisor->estaMuerto()) return;

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

void Nivel::onVolverClicked()
{
    // Volver al menú
    if (timer) timer->stop();
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

void Nivel::crearObstaculosAleatorios(int numExtraObst, qreal radioMin, qreal radioMax)
{
    // Centro del fondo
    auto fondoCentro = Vector2D(fondoSize.x() / 2.0, fondoSize.y() / 2.0);

    // Aleatorios
    for (int i = 0; i < numExtraObst; ++i) {
        qreal ang = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
        qreal r   = radioMin + (radioMax - radioMin) * QRandomGenerator::global()->generateDouble();
        Vector2D offset = Vector2D::desdePolar(r, ang);

        bool esCircular = (QRandomGenerator::global()->bounded(2) == 0);
        Obstaculo *obs = nullptr;

        if (esCircular) {
            qreal rad = 15.0 + QRandomGenerator::global()->generateDouble() * 20.0;
            obs = new Obstaculo(rad);
        } else {
            qreal w = 40.0 + QRandomGenerator::global()->generateDouble() * 60.0;
            qreal h = 20.0 + QRandomGenerator::global()->generateDouble() * 40.0;
            obs = new Obstaculo(w, h);
        }

        obs->setParentItem(fondoScroll);
        obs->setPos(fondoCentro.x() + offset.x(), fondoCentro.y() + offset.y());
        obstaculos.push_back(obs);
    }
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
    int gruposEnRonda = 0;
    int gruposTerminados = 0;

    for (Agente *a : agentes) {
        if (!a) continue;
        if (a->getRondaAsignada() != ronda_act) continue;
        gruposEnRonda++;
        if (a->rondaCompletada()) gruposTerminados++;
    }

    // Pasar de ronda
    if (gruposEnRonda > 0 && gruposTerminados == gruposEnRonda) {
        ronda_act++;
        // TODO: feedback UI "Ronda superada"
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

