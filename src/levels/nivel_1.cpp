#include "nivel_1.h"
#include <QVBoxLayout>
#include <QGraphicsRectItem>
#include <QBrush>

Nivel_1::Nivel_1(int numeroNivel, QWidget *parent)
    : QWidget(parent),
    numNivel(numeroNivel),
    jugador(nullptr),
    speed(2),
    IA(new OleadaAt_Colectivo()),
    m_sonidoAmbiente(nullptr)
{
    inicializarUI();
    inicializarEscena();
    cargarElementosNivel();
    inicializarHUD();
    cargarSonidos();

    // Iniciar bucle juego
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Nivel_1::actualizarJuego);
    timer->start(1500);
}

Nivel_1::~Nivel_1()
{
    timer->stop();

    // Limpia sonidos
    if (m_sonidoAmbiente) {
        m_sonidoAmbiente->stop();
        delete m_sonidoAmbiente;
        m_sonidoAmbiente = nullptr;
    }

    // Limpiar
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

    tiempodisparos = new QTimer();

    // Configurar vista
    vista->setScene(escena);
    vista->setRenderHint(QPainter::Antialiasing);
    vista->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Botón volver
    btnVolver = new QPushButton("Volver al Menú", this);
    btnVolver->setGeometry(10, 750, 120, 30);
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
    jugador = new Avion();
    enemigos.push_back(jugador);
    escena -> addItem(jugador);
}

void Nivel_1::actualizarJuego()
{
    // Bucle principal
    // 1. Actualizar IA y Actualizar proyectiles
    if (generarEnemigos() && IA -> Generarnewround()){
        int cont = 0; bool espacio; short int totalaviones; short int posy;
        totalaviones = QRandomGenerator::global()->bounded(1, 5);
        while (cont < totalaviones){
            //qDebug() << cont;
            espacio = true;
            posy = QRandomGenerator::global()->bounded(0, 326);
            for (Avion* enemy : enemigos){
                if (enemy -> pos().x() == 1495){
                    if (std::abs(posy - enemy -> pos().y()) < 35){
                        espacio = false;
                        break;
                    }
                }
            }

            if (espacio){
                Avion* enemigo = new Avion(false, 1495, posy);
                enemigos.push_back(enemigo);
                escena -> addItem(enemigo);
                if (IA -> getRondaActual() > 1){
                    enemigo -> setVelocidad(enemigo -> getVelocidad() + 1.5);
                }
                cont++;
            }
        }
    }
    if ((((IA -> Generarnewround() && !HayIA) && (!generarEnemigos() && enemigos.size() <= 1)) || (HayIA && IA -> GenerarnuevaOleada()))){
        qDebug() << derribados;
        generarIA();
    }

    connect(tiempodisparos, &QTimer::timeout, this, &Nivel_1::DisparosEnemigosAuto);
    tiempodisparos -> start(850);
}

bool Nivel_1::actualizarIA()
{
    if (IA != nullptr) {
        for (auto it = IA -> getGrupo().begin(); it != IA -> getGrupo().end();){
            Avion* EnemigoIA = dynamic_cast<Avion*>(*it);
            if (!EnemigoIA -> esJugador() && EnemigoIA -> estaMuerto()){
                it = IA -> getGrupo().erase(it);
                delete EnemigoIA;
            } else{
                it++;
            }
        }
        return true;
    } else {
        return false;
    }
}

void Nivel_1::manejarColisiones(Avion* ente)
{
    // Usar QGraphicsScene::collidingItems()
    // O implementar tu propia detección según el diagrama
    //Mejorar Conteo de Muertes
    if (!ente -> muerto){
        if (!(ente -> Planes_colision(jugador))){
            for (auto balasreceptor : ente -> getmunicion()){
                if (!balasreceptor -> muerto){
                    balasreceptor -> Colision_Avion();
                    if (ente -> muerto) {
                        jugador -> setderribados(jugador -> getderribados()+1);
                        break;
                    } else {
                        for (auto balasjugador : jugador -> getmunicion()){
                            balasjugador -> Colision_Balas(balasreceptor);
                        }
                    }
                }
            }
        }
    }
}

void Nivel_1::keyPressEvent(QKeyEvent *event)
{
    // Manejar controles del jugador
    if (HayIA){
        jugador -> setVelocidad(3);
    } else {
        jugador -> setVelocidad(5);
    }
    switch(event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up:
        // Mover arriba
        m_moverarriba = true;
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        // Mover abajo
        m_moverabajo = true;
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        // Mover izquierda
        m_moverizquierda = true;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        // Mover derecha
        m_moverderecha = true;
        break;
    case Qt::Key_E:
    case Qt::Key_N:
        //Disparo Parabólico
        jugador->Disparar(true);
        escena -> addItem(jugador -> obtenerDisparo(jugador -> getCantMunicion()));
        jugador -> setCantmunicion(1);
        break;
    case Qt::Key_Space:
        // Disparar
        jugador->Disparar(false);
        escena -> addItem(jugador -> obtenerDisparo(jugador -> getCantMunicion()));
        jugador -> setCantmunicion(1);
        break;
    }

    QWidget::keyPressEvent(event);
}

void Nivel_1::keyReleaseEvent(QKeyEvent *event)
{
    // Manejar liberación de teclas
    switch(event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up:
        // Mover arriba
        m_moverarriba = false;
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        // Mover abajo
        m_moverabajo = false;
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        // Mover izquierda
        m_moverizquierda = false;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        // Mover derecha
        m_moverderecha = false;
        break;
    }

    QWidget::keyReleaseEvent(event);
}

void Nivel_1::onVolverClicked(){
    timer->stop(); // Detener el juego

    // Detener música del nivel y desconectar señal
    if (m_sonidoAmbiente) {
        disconnect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                   this, &Nivel_1::onSonidoAmbienteTerminado);
        m_sonidoAmbiente->stop();
    }
    emit volverAlMenu();
}

void Nivel_1::update()
{
    foto1->moveBy(-speed, 0);
    foto2->moveBy(-speed, 0);

    if (m_moverabajo){
        jugador -> Mov_down();
    }
    if (m_moverarriba){
        jugador -> Mov_up();
    }
    if (m_moverderecha && !HayIA){
        jugador -> Mov_derecha();
    }
    if (m_moverizquierda && !HayIA){
        jugador -> Mov_izquierda();
    }

    for (auto it = enemigos.begin(); it != enemigos.end();) {
        Avion* e = *it;
        if (!(e -> esJugador())) {
            if (HayIA){
                QTimer::singleShot(500, [e](){
                    e -> Mov_Combinado();
                });
            } else {
                e -> Mov_izquierda();
            }
        }

        for (auto ite = e -> getmunicion().begin(); ite != e -> getmunicion().end(); ite++){
            Misil* misil = *ite;
            if (misil != nullptr){
                if (misil -> muerto){
                    escena -> removeItem(misil);
                } else {
                    misil -> avanzar();
                }
            }
        }

        manejarColisiones(e);

        if (e -> estaMuerto()){
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

    actualizarHUD();
    //Reorganizacion Imagenes en el Fondo.
    if (foto1->x() + width < 0){
        foto1->setX(foto2->x() + width);
    }
    if (foto2->x() + width < 0){
        foto2->setX(foto1->x() + width);
    }

    if (jugador -> muerto || !IA -> Generarnewround()){
        tiempodisparos -> stop();
        timer1 -> stop();
        timer -> stop();
        MostrarResultadosdeJuego();
    }
}

void Nivel_1::DisparosEnemigosAuto(){
    if(!HayIA){
        short int limit = 35;
        for (auto it = enemigos.begin(); it != enemigos.end(); it++) {
            Avion* avion = *it;
            if (!avion -> esJugador()){
                if (!avion -> getrecargar()){
                    if (std::abs(jugador -> pos().y() - avion -> pos().y()) < limit && std::abs(jugador -> pos().x() - avion -> pos().x()) < 900){
                        avion -> Disparar(false);
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
    qDebug() << "IA Generada";
    qDebug() << IA -> getCantOleadas() << "| " << IA -> getOleadaActual();
    HayIA = true;

    for (auto it = IA -> getGrupo().begin(); it != IA -> getGrupo().end(); it++){
        Avion* EnemigoIA = dynamic_cast<Avion*>(*it);
        escena -> addItem(EnemigoIA);
        enemigos.push_back(EnemigoIA);
    }
    qDebug() << "IA agregada a vector de enemigos.";

    if (IA -> getOleadaActual() > 1){
        OleadasSuperadas++;
        IA -> actualizar();
        qDebug() << "IA reposicionada.";
    }

    TotalOleadas++;

    if (IA -> rondaCompletada()){
        OleadasSuperadas++;
        RondasSuperadas++;
        qDebug() << "Enemigos IA Eliminados.";
        IA -> setRondas();
        qDebug() << IA -> getTotalRondas() << "| " << IA -> getRondaActual();
        QTimer::singleShot(3000, [this](){
            actualizarIA();
            limit_derribados += 5;
            HayIA = false;
        });
    }
}

bool Nivel_1::generarEnemigos(){
    return jugador -> getderribados() < limit_derribados;
}

void Nivel_1::MostrarResultadosdeJuego()
{
    // Detener musica
    if (m_sonidoAmbiente) {
        disconnect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                   this, &Nivel_1::onSonidoAmbienteTerminado);
        m_sonidoAmbiente->stop();
    }

    QColor colorOscuro(0, 0, 0, 150); // Negro con 150/255 de opacidad (~60% opaco)

    // Obtener los límites visibles de la escena
    QRectF sceneRect = QRectF(0, -400, 1600, 1500); // Asumiendo que Nivel_1 es la escena o la conoce

    // Crear y añadir el rectángulo oscuro que cubre la escena
    QGraphicsRectItem* darkener = new QGraphicsRectItem(sceneRect);
    darkener -> setBrush(QBrush(colorOscuro));
    darkener -> setZValue(999); // Asegura que esté por encima de todos los elementos del nivel
    escena -> addItem(darkener);
    // ================== EVALUAR OBJETIVOS (SE MANTIENE IGUAL) ==================
    int objetivosCumplidos = 0;
    short int LimiteEnemigos = jugador->getCreados() * 0.3;
    int LimiteDanio = jugador->getderribados() * 400; // Ajusta según tu juego

    bool sobrevivir = !jugador->muerto;
    bool eliminarEnemigos = jugador->getderribados() >= LimiteEnemigos;
    bool generarDanio = jugador->getDanioInfligido() > LimiteDanio;

    if (sobrevivir) objetivosCumplidos++;
    if (eliminarEnemigos) objetivosCumplidos++;
    if (generarDanio) objetivosCumplidos++;

    bool gano = (objetivosCumplidos >= 3); // Ajusta según tus reglas

    // Se mantiene esta variable aunque no se use en el layout final, por si la necesitas
    int porcentajeEnemigos = 0;
    if (LimiteEnemigos > 0) {
        porcentajeEnemigos = (jugador->getderribados() * 100) / LimiteEnemigos;
        if (porcentajeEnemigos > 100) porcentajeEnemigos = 100;
    }

    // ================== DIALOGO (ESTILO VISUAL MODIFICADO) ==================
    QDialog dialog(this);
    dialog.setWindowTitle("Resultados del nivel");
    dialog.setModal(true);
    // Tamaño ajustado para que quepa todo el contenido original con el estilo oscuro
    dialog.setFixedSize(450, 480);

    // Aplicación de estilo oscuro para el diálogo y sus widgets
    dialog.setStyleSheet(
        "QDialog { "
        "background-color: #1a1a1a; " // Fondo de la ventana oscuro
        "border: 1px solid #333333; " // Borde sutil
        "border-radius: 8px; "
        "} "
        "QLabel, QCheckBox { color: #E0E0E0; } " // Texto blanco/gris claro para todo el contenido
        "QCheckBox::indicator { width: 14px; height: 14px; } "
        // Estilo para las líneas divisorias
        "QFrame[frameShape=\"4\"] { background-color: #555555; height: 1px; }"
        "QPushButton { "
        "background-color: #333333; " // Botones oscuros
        "color: #FFFFFF; "
        "border: 1px solid #555555; "
        "border-radius: 5px; "
        "padding: 10px 25px; " // Botones grandes, estilo de la imagen
        "font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #444444; } "
        );

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(5);

    // ================== TITULO (CONTENIDO ORIGINAL RESTAURADO) ==================
    QLabel* titulo = new QLabel(gano ? "🏆 NIVEL SUPERADO" : "💀 MISIÓN FALLIDA");
    titulo->setAlignment(Qt::AlignCenter);
    QFont f = titulo->font();
    f.setPointSize(22); // Tamaño grande para el título
    f.setBold(true);
    titulo->setFont(f);
    // Mantiene colores originales (gold/red) pero sobre un bloque negro para el contraste
    titulo->setStyleSheet(
        gano ? "color: gold; background-color: #000000; padding: 10px 0; margin: 0;"
             : "color: red; background-color: #000000; padding: 10px 0; margin: 0;"
    );

    // ================== FUNCION LINEA (CONTENIDO ORIGINAL RESTAURADO) ==================
    auto crearLinea = []() {
        QFrame* linea = new QFrame();
        linea->setFrameShape(QFrame::HLine);
        linea->setFrameShadow(QFrame::Plain);
        linea->setStyleSheet("background-color: #333333; height: 1px;");
        linea->setFixedHeight(2);
        return linea;
    };

    // ================== RESUMEN (CONTENIDO ORIGINAL RESTAURADO) ==================
    QLabel* lblResumenTitulo = new QLabel("📊 Resumen");
    lblResumenTitulo->setStyleSheet("font-weight:bold; font-size: 14px; color: #CCCCCC; margin-top: 10px;");

    QLabel* resumen = new QLabel(QString("Objetivos cumplidos: %1 / 3").arg(objetivosCumplidos));
    resumen->setAlignment(Qt::AlignCenter);

    // ================== MISIONES (CONTENIDO ORIGINAL RESTAURADO) ==================
    QLabel* lblMisionesTitulo = new QLabel("🎯 Misiones");
    lblMisionesTitulo->setStyleSheet("font-weight:bold; font-size: 14px; color: #CCCCCC; margin-top: 10px;");

    QCheckBox* cbSobrevivir = new QCheckBox("Sobrevivir el nivel");
    cbSobrevivir->setChecked(sobrevivir);
    cbSobrevivir->setEnabled(false);
    // Se mantienen los colores originales de estado
    cbSobrevivir->setStyleSheet(sobrevivir ? "color: #00C853;" : "color: #FF5252;");

    QCheckBox* cbEliminar = new QCheckBox(
        QString("Eliminar enemigos (%1 / %2)").arg(jugador->getderribados()).arg(LimiteEnemigos)
        );
    cbEliminar->setChecked(eliminarEnemigos);
    cbEliminar->setEnabled(false);
    cbEliminar->setStyleSheet(eliminarEnemigos ? "color: #00C853;" : "color: #FF5252;");

    QCheckBox* cbDanio = new QCheckBox(
        QString("Generar el mayor daño posible (%1 / %2)").arg(jugador->getDanioInfligido()).arg(LimiteDanio)
    );
    cbDanio->setChecked(generarDanio);
    cbDanio->setEnabled(false);
    cbDanio->setStyleSheet(generarDanio ? "color: #00C853;" : "color: #FF5252;");

    // ================== PROGRESO DEL NIVEL (CONTENIDO ORIGINAL RESTAURADO) ==================
    QLabel* lblProgresoTitulo = new QLabel("⏱ Progreso del nivel");
    lblProgresoTitulo->setStyleSheet("font-weight: bold; font-size: 14px; color: #CCCCCC; margin-top: 10px;");

    QLabel* lblRondas = new QLabel(
        QString("Rondas sobrevividas: %1 / %2").arg(RondasSuperadas).arg(IA->getTotalRondas())
    );

    QLabel* lblOleadas = new QLabel(
        QString("Oleadas superadas: %1 / %2").arg(OleadasSuperadas).arg(TotalOleadas)
    );

    // ================== BOTONES (CONTENIDO ORIGINAL RESTAURADO, ESTILO VISUAL MODIFICADO) ==================
    QPushButton* btnReintentar = new QPushButton("Reintentar");
    QPushButton* btnSalir = new QPushButton("Salir al menú"); // Se mantiene el texto original

    QHBoxLayout* botones = new QHBoxLayout();
    botones->addStretch(); // Para centrar los botones
    botones->addWidget(btnReintentar);
    botones->addWidget(btnSalir);
    botones->addStretch();

    // ================== ARMAR LAYOUT (CON TODA LA ESTRUCTURA ORIGINAL) ==================
    layout->addWidget(titulo);
    layout->addSpacing(10);

    // RESUMEN
    layout->addWidget(lblResumenTitulo);
    layout->addWidget(crearLinea());
    layout->addWidget(resumen);
    layout->addSpacing(8);

    // MISIONES
    layout->addWidget(lblMisionesTitulo);
    layout->addWidget(crearLinea());
    layout->addWidget(cbSobrevivir);
    layout->addWidget(cbEliminar);
    layout->addWidget(cbDanio);
    layout->addSpacing(8);

    // PROGRESO
    layout->addWidget(lblProgresoTitulo);
    layout->addWidget(crearLinea());
    layout->addWidget(lblRondas);
    layout->addWidget(lblOleadas);

    layout->addStretch(); // Empuja los botones hacia abajo
    layout->addLayout(botones);


    // ================== CONEXIONES Y DECISIÓN ==================
    connect(btnReintentar, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(btnSalir, &QPushButton::clicked, &dialog, &QDialog::reject);

    int result = dialog.exec();

       // 3. Eliminar la capa oscura después de cerrar el diálogo
    if (darkener) {
        escena->removeItem(darkener);
        delete darkener;
        darkener = nullptr;
    }

    if (result == QDialog::Accepted) {
        reiniciarNivel();
    } else {
        emit volverAlMenu();
    }
}

void Nivel_1::reiniciarNivel(){
    // Limpiar escena
    escena->clear();

    // Reset de estado
    derribados = 0;

    // Volver a crear todo
    inicializarUI();
    inicializarEscena();
    cargarElementosNivel();

    // Reactivar musica
    if (m_sonidoAmbiente) {
        connect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                this, &Nivel_1::onSonidoAmbienteTerminado);
        if (!m_sonidoAmbiente->isPlaying()) {
            m_sonidoAmbiente->play();
        }
    }

    // Reactivar timers
    timer->start();
    timer1->start();
    tiempodisparos->start();
}

void Nivel_1::inicializarHUD(){
    barraVida = new QProgressBar(this); // hijo directo de la ventana
    barraVida->setRange(0, jugador->getVida());
    barraVida->setValue(jugador->getVida());
    barraVida->setTextVisible(true);
    barraVida->setFormat("Vida: %v / %m"); // texto dentro de la barra
    barraVida->setAlignment(Qt::AlignCenter);

    // Alineación a la izquierda (10px de margen)
    barraVida->setGeometry(10, 10, 200, 25);

    // Estilo: borde negro, fondo gris, relleno verde
    barraVida->setStyleSheet(
        "QProgressBar {"
        "  border: 2px solid black;"
        "  border-radius: 5px;"
        "  background-color: #95a5a6;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #2ecc71;"
        "  border-radius: 3px;"
        "}"
        );

    barraVida->show();

    QWidget *contenedorStats = new QWidget(this);

    // Definiciones de tamaño
    int anchoLabel = 150;
    int altoLabel = 25;
    int margen = 10;
    int y = margen; // Posición Y arriba

    int x = 900;

    // **2. Crear el Layout Horizontal (QHBoxLayout)**
    QHBoxLayout *layoutHorizontal = new QHBoxLayout(contenedorStats);
    layoutHorizontal->setSpacing(5); // Espacio entre labels
    layoutHorizontal->setContentsMargins(5, 5, 5, 5); // Margen interior del contenedor

    // 3. Estilo para los Labels individuales
    QString estiloLabel =
        "QLabel {"
        "  border: 1px solid #7f8c8d;"                       // Borde gris/plata más sutil
        "  border-radius: 4px;"
        "  background-color: rgba(236, 240, 241, 150);"      // Gris muy claro semi-transparente
        "  color: #2c3e50;"                                  // Letra azul oscuro/negro para contraste
        "  font-weight: bold;"
        "  text-align: center;"
        "}";

    // **4. Estilo para el Contenedor de Fondo (QWidget)**
    // Usaremos un fondo para que el conjunto de labels resalte.
    QString estiloContenedor =
        "QWidget {"
        "  background-color: rgba(44, 62, 80, 200);"          // Fondo oscuro (azul marino/gris oscuro) semi-transparente
        "  border: 2px solid #ecf0f1;"                       // Borde blanco/plata
        "  border-radius: 8px;"
        "}";

    // **5. Inicializar y configurar los Labels**
    // ... (Inicialización de lblRonda, lblOleadas, lblEnemigos, lblDanio)

    // Ronda
    lblRonda = new QLabel(QString("Ronda: %1 / %2").arg(IA -> getRondaActual()).arg(IA -> getTotalRondas()));
    lblRonda->setMinimumSize(anchoLabel, altoLabel); // Forzar tamaño
    lblRonda->setAlignment(Qt::AlignCenter);
    lblRonda->setStyleSheet(estiloLabel);
    layoutHorizontal->addWidget(lblRonda); // Agregar al layout

    // Oleadas
    lblOleadas = new QLabel(QString("Oleadas: %1 / %2").arg(IA -> getOleadaActual()).arg(IA -> getCantOleadas() - 1));
    lblOleadas->setMinimumSize(anchoLabel, altoLabel);
    lblOleadas->setAlignment(Qt::AlignCenter);
    lblOleadas->setStyleSheet(estiloLabel);
    layoutHorizontal->addWidget(lblOleadas);

    // Enemigos derribados
    lblEnemigos = new QLabel(QString("Derribados: %1").arg(jugador->getderribados()));
    lblEnemigos->setMinimumSize(anchoLabel, altoLabel);
    lblEnemigos->setAlignment(Qt::AlignCenter);
    lblEnemigos->setStyleSheet(estiloLabel);
    layoutHorizontal->addWidget(lblEnemigos);

    // Daño infligido
    lblDanio = new QLabel(QString("Daño: %1").arg(jugador->getDanioInfligido()));
    lblDanio->setMinimumSize(anchoLabel, altoLabel);
    lblDanio->setAlignment(Qt::AlignCenter);
    lblDanio->setStyleSheet(estiloLabel);
    layoutHorizontal->addWidget(lblDanio);

    // **6. Aplicar estilos y posicionar el Contenedor**
    contenedorStats->setStyleSheet(estiloContenedor);

    // Obtener el ancho y alto que el layout necesita
    int containerWidth = contenedorStats->sizeHint().width();
    int containerHeight = contenedorStats->sizeHint().height();

    // Posicionar el contenedor completo con X = 850
    contenedorStats->setGeometry(x, y, containerWidth, containerHeight);

    // **7. Asegurar que esté por encima de todo**
    contenedorStats->raise();
    contenedorStats->show();
}

void Nivel_1::actualizarHUD() {
    // 1. -------- Actualizar Vida y Color de la Barra de Progreso --------
    int vidaActual = jugador->getVida();
    barraVida->setValue(vidaActual);

    // Calcular el porcentaje de vida para determinar el color
    double porcentaje = (double)vidaActual / barraVida->maximum() * 100;
    QString color;

    // Asignación de color:
    if (porcentaje > 70)
        color = "#2ecc71";  // Verde (Vida alta)
    else if (porcentaje > 30)
        color = "#f1c40f";  // Amarillo (Vida media)
    else
        color = "#e74c3c";  // Rojo (Vida baja)

    // Aplicar el estilo CSS al 'chunk' (relleno) de la barra
    barraVida->setStyleSheet(QString(
                                 "QProgressBar {"
                                 "  border: 2px solid black;" // Mantener el borde si el original lo tiene
                                 "  border-radius: 5px;"     // Mantener el radio
                                 "  background-color: #95a5a6;"
                                 "  text-align: center;"
                                 "}"
                                 "QProgressBar::chunk {"
                                 "  background-color: %1;"
                                 "  border-radius: 3px;"
                                 "}"
                                 ).arg(color));

    // 2. -------- Actualizar Estadísticas de Texto (Labels) --------

    // Ronda
    lblRonda->setText(QString("Ronda: %1 / %2").arg(IA -> getRondaActual()).arg(IA -> getTotalRondas()));

    // Oleadas
    lblOleadas->setText(QString("Oleadas: %1 / %2").arg(IA -> getOleadaActual()).arg(IA -> getCantOleadas() - 1));

    // Enemigos derribados
    // Nota: Es crucial que 'jugador->getderribados()' sea el valor actualizado
    lblEnemigos->setText(QString("Enemigos: %1").arg(jugador->getderribados()));

    // Daño infligido
    // Nota: Es crucial que 'jugador->getDanioInfligido()' sea el valor actualizado
    lblDanio->setText(QString("Daño: %1").arg(jugador->getDanioInfligido()));
}

// Sistema de sonido
void Nivel_1::cargarSonidos()
{
    cargarSonidosDesdeArchivos();
}

void Nivel_1::cargarSonidosDesdeArchivos()
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
    QString rutaAmbiente = rutaEncontrada + "/musica_nivel1.wav";
    m_sonidoAmbiente = new QSoundEffect(this);

    if (QFile::exists(rutaAmbiente)) {
        m_sonidoAmbiente->setSource(QUrl::fromLocalFile(rutaAmbiente));
        m_sonidoAmbiente->setVolume(0.1f);
        m_sonidoAmbiente->setLoopCount(1);  // Solo una reproducción por vez

        // Conectar señal para reproducir en loop cuando termine
        connect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                this, &Nivel_1::onSonidoAmbienteTerminado);

        // Iniciar reproducción
        m_sonidoAmbiente->play();

    } else {
        delete m_sonidoAmbiente;
        m_sonidoAmbiente = nullptr;
    }
}

void Nivel_1::onSonidoAmbienteTerminado()
{
    // Este slot se llama cada vez que cambia el estado de reproducción
    if (m_sonidoAmbiente && !m_sonidoAmbiente->isPlaying()) {
        // Si el sonido terminó de reproducirse, reproducirlo nuevamente
        // (esto crea un loop continuo)
        m_sonidoAmbiente->play();
    }
}
