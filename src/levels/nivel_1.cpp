#include "nivel_1.h"
#include <QVBoxLayout>
#include <QGraphicsRectItem>
#include <QBrush>

Nivel_1::Nivel_1(int numeroNivel, QWidget *parent)
    : QWidget(parent),
    numNivel(numeroNivel),
    jugador(nullptr),
    speed(2),
    IA(new OleadaAt_Colectivo())
{
    inicializarUI();
    inicializarEscena();
    cargarElementosNivel();

    // Iniciar bucle juego
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Nivel_1::actualizarJuego);
    timer->start(1500);
}

Nivel_1::~Nivel_1()
{
    timer->stop();

    // Limpiar
    for (auto p : participantes) {
        delete p;
    }

    if (jugador) delete jugador;
    if (IA) delete IA;
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

    foto1 = escena->addPixmap(foto);
    foto = foto.transformed(QTransform().scale(-1, 1));
    foto2 = escena->addPixmap(foto);

    foto1->setPos(0, 0);
    foto2->setPos(width, 0);

    // Timer para actualizar
    timer1 = new QTimer(this);
    connect(timer1, &QTimer::timeout, this, &Nivel_1::update);
    timer1->start(16);

    tiempodesplazarelementos = new QTimer();

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
}

void Nivel_1::cargarElementosNivel()
{
    // AQUÍ irá la lógica para cargar
    jugador = new Avion(35, 70, 170, true, 7);
    enemigos.push_back(jugador);
    escena -> addItem(jugador);
}

void Nivel_1::actualizarJuego()
{
    // Bucle principal
    // 1. Actualizar IA y Actualizar proyectiles
    if (!HayIA){
        int cont = 0; bool espacio; short int totalaviones; short int posy;
        totalaviones = QRandomGenerator::global()->bounded(1, 5);
        while (cont < totalaviones){
            //qDebug() << cont;
            espacio = true;
            posy = QRandomGenerator::global()->bounded(0, 326);
            //qDebug() << jugador->getCreados()
            //qDebug() << jugador->getCreados() << "|" << enemigos.size();
            for (Avion* enemy : enemigos){
                if (enemy -> getx() == 1525){
                    if (std::abs(posy - enemy -> gety()) < 35){
                        espacio = false;
                        break;
                    }
                }
            }

            if (espacio){
                Avion* enemigo = new Avion(35, 1525, posy);
                enemigos.push_back(enemigo);
                escena -> addItem(enemigo);
                cont++;
            }
        }
    }
    if (((IA -> Generarnewround() && !HayIA) && (jugador -> getderribados()%5 == 0 && jugador -> getderribados() != 0)) || (HayIA && actualizarIA())){
        generarIA();
    }

    connect(tiempodesplazarelementos, &QTimer::timeout, this, &Nivel_1::DisparosEnemigosAuto);
    tiempodesplazarelementos -> start(500);

    // 3. Verificar condiciones de victoria/
    if (jugador -> getVida() == 0){
        tiempodesplazarelementos -> stop();
        timer1 -> stop();
        timer -> stop();
    }
}

bool Nivel_1::actualizarIA()
{
    if (IA != nullptr) {
        for (auto it = IA -> getGrupo().begin(); it != IA -> getGrupo().end();){
            Avion* EnemigoIA = dynamic_cast<Avion*>(*it);
            if (!EnemigoIA -> esJugador() && EnemigoIA -> getdestruido()){
                it = IA -> getGrupo().erase(it);
                //delete EnemigoIA;
            } else{
                it++;
            }
        }
        return true;
    } else {
        return false;
    }
}

void Nivel_1::manejarColisiones(Avion* emisor)
{
    // Usar QGraphicsScene::collidingItems()
    // O implementar tu propia detección según el diagrama
    for (auto it = enemigos.begin(); it != enemigos.end(); it++){
        Avion* avionactual = *it;
        if (emisor != avionactual){
            if (!emisor -> Planes_colision(avionactual) && emisor -> esJugador() != avionactual -> esJugador()){
                for (auto balasavionactual : avionactual -> getmunicion()){
                    if (balasavionactual != nullptr && std::abs(emisor -> gety() - balasavionactual -> gety()) < 35){
                        for (auto balaavion : emisor -> getmunicion()){
                            if (std::abs(balaavion -> gety() - balasavionactual -> gety()) < 7){
                                if (balaavion -> Colision_Balas(balasavionactual)){
                                    break;
                                }
                            }
                        }
                        if (emisor -> Balas_colision(balasavionactual)){
                            break;
                        }
                    }
                }
            }
        }
        if (avionactual -> getVida() == 0 && !avionactual -> esJugador() && !HayIA){
            jugador -> setderribados();
        }
    }
}

void Nivel_1::keyPressEvent(QKeyEvent *event)
{
    // Manejar controles del jugador
    switch(event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up:
        // Mover arriba
        jugador->setVelocidady(-jugador -> getVelocidad());
        jugador -> Mov_vertical();
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        // Mover abajo
        jugador->setVelocidady(jugador -> getVelocidad());
        jugador -> Mov_vertical();
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        // Mover izquierda
        jugador->Mov_izquierda();
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        // Mover derecha
        jugador->Mov_derecha();
        break;
    case Qt::Key_Space:
        // Disparar
        jugador->Disparar();
        escena -> addItem(jugador -> obtenerDisparo(jugador -> getCantMunicion()));
        jugador -> setCantmunicion(1);
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
    foto1->moveBy(-speed, 0);
    foto2->moveBy(-speed, 0);

    for (auto it = enemigos.begin(); it != enemigos.end();) {
        Avion* e = *it;
        if (!(e -> esJugador())) {
            if (HayIA){
                e -> Mov_vertical();
            }
            e -> Mov_izquierda();
        }

        for (auto ite = e -> getmunicion().begin(); ite != e -> getmunicion().end(); ite++){
            Misil* misil = *ite;
            if (misil != nullptr){
                misil -> Desplazar(e);
                if (misil ->getcrashed()){
                    escena -> removeItem(misil);
                }
            }
        }
        manejarColisiones(e);
        if (e -> getdestruido()){
            escena->removeItem(e);
            it = enemigos.erase(it);
            if (!HayIA){
                delete e;
            }
        } else {
            ++it;
        }
    }
    //Actualizo Municion de cada Avion en Juego.
    for (auto AvionenJuego : enemigos){
        AvionenJuego -> actualizarelementos();
    }

    //Reorganizacion Imagenes en el Fondo.
    if (foto1->x() + width < 0){
        foto1->setX(foto2->x() + width);
    }
    if (foto2->x() + width < 0){
        foto2->setX(foto1->x() + width);
    }
}

void Nivel_1::DisparosEnemigosAuto(){
    if(!HayIA){
        short int limit = 35;
        for (auto it = enemigos.begin(); it != enemigos.end(); it++) {
            Avion* avion = *it;
            if (!avion -> esJugador()){
                if (!avion -> getrecargar()){
                    if (std::abs(jugador -> gety() - avion -> gety()) < limit && std::abs(jugador -> getx() - avion -> getx()) < 900){
                        avion -> Disparar();
                        escena -> addItem(avion -> obtenerDisparo(avion -> getCantMunicion()));
                        avion -> setCantmunicion(1);
                    }
                }
            }
        }
    }
}

void Nivel_1::generarIA(){
    IA -> Generarenemigos();
    IA -> Calcular_Desplazamiento(jugador);
    if (!HayIA){
        for (auto it = enemigos.begin(); it != enemigos.end();){
            Avion* enemigo = *it;
            if (!enemigo -> esJugador()){
                escena -> removeItem(enemigo);
                it = enemigos.erase(it);
                delete enemigo;
            } else {
                it++;
            }
        }
    }
    HayIA = true;
    //qDebug() << "IA generada";
    for (auto it = IA -> getGrupo().begin(); it != IA -> getGrupo().end(); it++){
        Avion* EnemigoIA = dynamic_cast<Avion*>(*it);
        escena -> addItem(EnemigoIA);
        enemigos.push_back(EnemigoIA);
    }
    if (IA -> rondaCompletada()){
        HayIA = false;
        IA -> setRondas();
        jugador -> setderribados(-5);
    }
}
