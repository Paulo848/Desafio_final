#include "nivel_1.h"
#include <QVBoxLayout>
#include <QGraphicsRectItem>
#include <QBrush>

Nivel_1::Nivel_1(int numeroNivel, QWidget *parent)
    : QWidget(parent),
    numNivel(numeroNivel),
    jugador(nullptr),
    speed(2),
    ia(nullptr)
{
    inicializarUI();
    inicializarEscena();
    cargarElementosNivel();

    // Iniciar bucle juego
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Nivel_1::actualizarJuego);
    timer->start(500);
}

Nivel_1::~Nivel_1()
{
    timer->stop();

    // Limpiar
    for (auto p : participantes) {
        delete p;
    }
    for (auto o : obstaculos) {
        delete o;
    }

    if (jugador) delete jugador;
    if (ia) delete ia;
}

void Nivel_1::inicializarUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Crear vista y escena
    vista = new QGraphicsView(this);
    escena = new QGraphicsScene(this);
    QPixmap foto(":/ui/ui/Fondo1.png");
    foto = foto.scaled(1600, 600, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    width = foto.width();

    bg1 = escena->addPixmap(foto);
    foto = foto.transformed(QTransform().scale(-1, 1));
    bg2 = escena->addPixmap(foto);

    bg1->setPos(0, 0);
    bg2->setPos(width, 0);

    // Timer para actualizar
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Nivel_1::update);
    timer->start(15);  //

    // Configurar vista
    vista->setScene(escena);
    vista->setRenderHint(QPainter::Antialiasing);
    vista->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Botón volver
    btnVolver = new QPushButton("Volver al Menú", this);
    btnVolver->setGeometry(10, 10, 120, 30);
    connect(btnVolver, &QPushButton::clicked, this, &Nivel_1::onVolverClicked);

    layout->addWidget(vista);
    setLayout(layout);

    setFocusPolicy(Qt::StrongFocus);
}

void Nivel_1::inicializarEscena()
{
    escena->setSceneRect(390, 0, 800, 600);

    // Fondo según el nivel
    switch(numNivel) {
    case 1:
        escena->setBackgroundBrush(QBrush(QColor(135, 206, 235))); // Cielo azul
        break;
    case 2:
        escena->setBackgroundBrush(QBrush(QColor(25, 25, 112))); // Azul oscuro (noche)
        break;
    case 3:
        escena->setBackgroundBrush(QBrush(QColor(70, 130, 180))); // Azul acero (mar)
        break;
    }

    // Agregar texto temporal
    QGraphicsTextItem *texto = escena->addText(
        QString("NIVEL 1 - barco").arg(numNivel)
        );
    texto->setDefaultTextColor(Qt::white);
    texto->setPos(200, 20);
    QFont font = texto->font();
    font.setPointSize(20);
    font.setBold(true);
    texto->setFont(font);
}

void Nivel_1::cargarElementosNivel()
{
    // AQUÍ irá la lógica para cargar
}

void Nivel_1::actualizarJuego()
{
    // Bucle principal

    // 1. Actualizar posiciones de participantes
    for (auto p : participantes) {
        // p->actualizar();
    }

    // 2. Actualizar IA
    actualizarIA();

    // 3. Detectar colisiones
    manejarColisiones();

    // 4. Actualizar proyectiles
    // ...

    // 5. Verificar condiciones de victoria/derrota
    // ...
}

void Nivel_1::actualizarIA()
{
    if (ia != nullptr) {
        // ia->Comportamiento();
    }
}

void Nivel_1::manejarColisiones()
{
    // Usar QGraphicsScene::collidingItems()
    // O implementar tu propia detección según el diagrama
}

void Nivel_1::keyPressEvent(QKeyEvent *event)
{
    // Manejar controles del jugador
    switch(event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up:
        // Mover arriba
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        // Mover abajo
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        // Mover izquierda
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        // Mover derecha
        break;
    case Qt::Key_Space:
        // Disparar
        // if (jugador) jugador->Disparar();
        break;
    }

    QWidget::keyPressEvent(event);
}

void Nivel_1::keyReleaseEvent(QKeyEvent *event)
{
    // Manejar liberación de teclas
    QWidget::keyReleaseEvent(event);
}

void Nivel_1::onVolverClicked()
{
    timer->stop(); // Detener el juego
    emit volverAlMenu();
}

void Nivel_1::update()
{
    bg1->moveBy(-speed, 0);
    bg2->moveBy(-speed, 0);

    // Si bg1 salió, va detrás de bg2
    qDebug() << bg1->x() << "|" << width;
    if (bg1->x() + width < 0){
        bg1->setX(bg2->x() + width);
    }

    // Si bg2 salió, va detrás de bg1
    if (bg2->x() + width < 0){
        bg2->setX(bg1->x() + width);
    }
}
