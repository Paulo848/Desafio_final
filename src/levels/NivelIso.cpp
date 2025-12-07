#include "NivelIso.h"

#include <QtMath>
#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QRectF>
#include <QPushButton>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <QFile>

#include "proyeccioniso.h"

NivelIso::NivelIso(QWidget *parent)
    : QWidget(parent),
    m_timer(new QTimer(this)),
    m_moveLeft(false),
    m_moveRight(false),
    m_sprint(false),
    m_scrollOffset(0.0),
    m_scrollSpeed(2.0),
    m_scrollSpeedBase(2.0),
    m_scrollSpeedSprint(4.5),
    m_limiteEliminacion(-200.0),
    m_limiteGeneracion(300.0),
    m_contadorFrames(0),
    m_vidas(4),
    m_vidasMaximas(4),
    m_invulnerable(false),
    m_contadorInvulnerabilidad(0),
    m_tiempoTranscurrido(0),
    m_tiempoParaGanar(1200),  // 1200 frames = 20 segundos a 60 FPS
    m_nivelCompletado(false),
    m_cooldownDisparo(15),    // 15 frames entre disparos (~0.25s)
    m_cooldownActual(0),
    m_frecuenciaGeneracion(60),
    m_cantidadObstaculos(2),
    m_municionActual(4),
    m_municionMaxima(4),
    m_contadorRecarga(0),
    m_tiempoRecarga(240),     // 240 frames = 4 segundos para recargar
    m_sonidoDisparo(nullptr),
    m_sonidoExplosion(nullptr)
{
    // Este widget necesita recibir eventos de teclado
    setFocusPolicy(Qt::StrongFocus);

    // Cargar sonidos y configurar escena inicial
    cargarSonidos();
    initScene();

    // Timer del game loop (aproximadamente 60 FPS)
    connect(m_timer, &QTimer::timeout, this, &NivelIso::updateGame);
    m_timer->start(16);

    // Botón para volver al menú principal
    QPushButton *btnVolver = new QPushButton("Volver", this);
    btnVolver->setGeometry(10, 10, 100, 30);
    connect(btnVolver, &QPushButton::clicked, this, [this]() {
        emit volverAlMenu();
    });
}

NivelIso::~NivelIso()
{
    // Limpiar recursos de sonido
    if (m_sonidoDisparo) {
        m_sonidoDisparo->stop();
        delete m_sonidoDisparo;
    }

    if (m_sonidoExplosion) {
        m_sonidoExplosion->stop();
        delete m_sonidoExplosion;
    }
}

void NivelIso::cargarSonidos()
{
    cargarSonidosDesdeArchivos();
}

void NivelIso::cargarSonidosDesdeArchivos()
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

    // Cargar sonido de disparo
    QString rutaDisparo = rutaEncontrada + "/sonido_de_disparo.wav";
    m_sonidoDisparo = new QSoundEffect(this);
    if (QFile::exists(rutaDisparo)) {
        m_sonidoDisparo->setSource(QUrl::fromLocalFile(rutaDisparo));
        m_sonidoDisparo->setVolume(2.8f);
        m_sonidoDisparo->setLoopCount(1);
    }

    // Cargar sonido de explosión
    QString rutaExplosion = rutaEncontrada + "/sonido_de_explosion.wav";
    m_sonidoExplosion = new QSoundEffect(this);
    if (QFile::exists(rutaExplosion)) {
        m_sonidoExplosion->setSource(QUrl::fromLocalFile(rutaExplosion));
        m_sonidoExplosion->setVolume(2.8f);
        m_sonidoExplosion->setLoopCount(1);
    }
}

QSize NivelIso::minimumSizeHint() const
{
    return QSize(640, 480);
}

QSize NivelIso::sizeHint() const
{
    return QSize(800, 600);
}

void NivelIso::initScene()
{
    // Barco FIJO en posición inicial (cerca del jugador, centrado lateralmente)
    m_barco.setPosition(QPointF(-150.0, 0.0));

    // Limpiar obstáculos previos
    m_obstaculos.clear();

    // Generar algunos obstáculos iniciales alejados del barco
    for (int i = 0; i < 3; i++) {
        Obstaculon2 o;
        qreal x = 100.0 + i * 80.0;  // Lejos del barco
        qreal y = static_cast<qreal>(QRandomGenerator::global()->bounded(-100, 100));
        o.setPosition(QPointF(x, y));
        m_obstaculos.append(o);
    }

    // Definir área jugable en coordenadas del mundo 2D
    // x = profundidad (obstáculos avanzan en -X)
    // y = ancho lateral (barco se mueve en Y)
    m_playArea = QRectF(-200.0, -150.0, 400.0, 300.0);
}

void NivelIso::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Dibujar fondo con efecto de scrolling
    dibujarFondoScrolling(painter);

    // Transformación: origen en centro de pantalla
    painter.translate(width() / 2.0, height() / 2.0);

    // Dibujar HUD (vidas, tiempo, munición) en coordenadas de ventana
    painter.save();
    painter.resetTransform();
    dibujarVidas(painter);
    dibujarTiempo(painter);
    dibujarMunicion(painter);
    painter.restore();

    // Restaurar transformación para elementos del juego
    painter.resetTransform();
    painter.translate(width() / 2.0, height() / 2.0);

    // Dibujar marco del área jugable
    if (!m_playArea.isNull()) {
        QPointF tl = m_playArea.topLeft();
        QPointF tr = m_playArea.topRight();
        QPointF br = m_playArea.bottomRight();
        QPointF bl = m_playArea.bottomLeft();

        // Proyectar esquinas a pantalla isométrica
        QVector<QPointF> pts;
        pts << ProyeccionIso::toScreen(tl)
            << ProyeccionIso::toScreen(tr)
            << ProyeccionIso::toScreen(br)
            << ProyeccionIso::toScreen(bl);

        painter.save();
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(QPolygonF(pts));
        painter.restore();
    }

    // Dibujar barco (con parpadeo si está invulnerable)
    QPointF barcoWorld = m_barco.position();
    QPointF barcoScreen = ProyeccionIso::toScreen(barcoWorld);

    bool dibujarBarco = true;
    if (m_invulnerable) {
        // Parpadear cada 5 frames
        dibujarBarco = (m_contadorInvulnerabilidad % 10 < 5);
    }

    if (dibujarBarco) {
        painter.save();
        painter.translate(barcoScreen);
        painter.setBrush(Qt::yellow);
        painter.setPen(Qt::black);
        painter.drawRect(-BARCO_PROFUNDIDAD/2, -BARCO_ANCHO/2, BARCO_PROFUNDIDAD, BARCO_ANCHO);
        painter.restore();
    }

    // Dibujar obstáculos
    for (const Obstaculon2 &o : m_obstaculos) {
        QPointF oWorld = o.position();
        QPointF oScreen = ProyeccionIso::toScreen(oWorld);

        painter.save();
        painter.translate(oScreen);
        painter.setBrush(Qt::gray);
        painter.setPen(Qt::black);
        painter.drawRect(-OBS_PROFUNDIDAD/2, -OBS_ANCHO/2, OBS_PROFUNDIDAD, OBS_ANCHO);
        painter.restore();
    }

    // Dibujar torpedos
    for (const Torpedo &t : m_torpedos) {
        if (t.estaActivo()) {
            QPointF tWorld = t.position();
            QPointF tScreen = ProyeccionIso::toScreen(tWorld);

            painter.save();
            painter.translate(tScreen);
            painter.setBrush(Qt::cyan);
            painter.setPen(Qt::darkCyan);
            painter.drawEllipse(QRectF(-8, -4, 16, 8));
            painter.restore();
        }
    }

    // Dibujar hitboxes de depuración (verde = no colisión, rojo = colisión)
    drawHitbox(painter, m_barco.hitbox(), m_barco.position());

    for (const Obstaculon2 &o : m_obstaculos) {
        drawHitbox(painter, o.hitbox(), o.position());
    }

    for (const Torpedo &t : m_torpedos) {
        if (t.estaActivo()) {
            drawHitbox(painter, t.hitbox(), t.position());
        }
    }
}

void NivelIso::dibujarVidas(QPainter &painter)
{
    // Dibujar corazones en la esquina superior izquierda
    int x = 120;
    int y = 15;
    int size = 20;
    int spacing = 25;

    painter.save();

    for (int i = 0; i < m_vidasMaximas; i++) {
        QRect corazonRect(x + i * spacing, y, size, size);

        if (i < m_vidas) {
            // Corazón lleno (vida disponible)
            painter.setBrush(Qt::red);
            painter.setPen(Qt::darkRed);
        } else {
            // Corazón vacío (vida perdida)
            painter.setBrush(Qt::darkGray);
            painter.setPen(Qt::gray);
        }

        painter.drawEllipse(corazonRect);
    }

    // Texto con cantidad de vidas
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(12);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(x + m_vidasMaximas * spacing + 10, y + 15,
                     QString("x %1").arg(m_vidas));

    painter.restore();
}

void NivelIso::dibujarMunicion(QPainter &painter)
{
    painter.save();

    // Posición debajo de las vidas
    int x = 120;
    int y = 50;
    int size = 18;
    int spacing = 22;

    // Título
    QFont fontTitulo = painter.font();
    fontTitulo.setPointSize(10);
    fontTitulo.setBold(true);
    painter.setFont(fontTitulo);
    painter.setPen(Qt::white);
    painter.drawText(x, y - 5, "Torpedos:");

    // Dibujar torpedos disponibles
    for (int i = 0; i < m_municionMaxima; i++) {
        QRect torpedoRect(x + i * spacing, y, size, size);

        if (i < m_municionActual) {
            // Torpedo disponible
            painter.setBrush(QColor(0, 255, 255));
            painter.setPen(QColor(0, 200, 255));
        } else {
            // Torpedo gastado
            painter.setBrush(Qt::darkGray);
            painter.setPen(Qt::gray);
        }

        painter.drawEllipse(torpedoRect);
    }

    // Contador numérico
    painter.setPen(Qt::cyan);
    QFont fontNum = painter.font();
    fontNum.setPointSize(12);
    fontNum.setBold(true);
    painter.setFont(fontNum);
    painter.drawText(x + m_municionMaxima * spacing + 5, y + 15,
                     QString("x %1").arg(m_municionActual));

    painter.restore();
}

void NivelIso::dispararTorpedo()
{
    // Verificar cooldown y munición disponible
    if (m_cooldownActual > 0 || m_municionActual <= 0) {
        return;
    }

    // Crear torpedo en la posición del barco
    Torpedo torpedo;
    QPointF posBarco = m_barco.position();
    torpedo.setPosition(QPointF(posBarco.x() + 20, posBarco.y()));
    m_torpedos.append(torpedo);

    // Activar cooldown y gastar munición
    m_cooldownActual = m_cooldownDisparo;
    m_municionActual--;

    // Si se acabó la munición, iniciar recarga automática
    if (m_municionActual == 0) {
        m_contadorRecarga = m_tiempoRecarga;
    }

    // Reproducir sonido
    if (m_sonidoDisparo) {
        m_sonidoDisparo->play();
    }
}

void NivelIso::updateTorpedos()
{
    // Actualizar posición y eliminar torpedos fuera del área
    for (int i = m_torpedos.size() - 1; i >= 0; --i) {
        m_torpedos[i].actualizar();

        // Eliminar torpedos que salieron del mapa
        if (m_torpedos[i].position().x() > m_limiteGeneracion + 100) {
            m_torpedos.removeAt(i);
        } else if (!m_torpedos[i].estaActivo()) {
            m_torpedos.removeAt(i);
        }
    }
}

void NivelIso::verificarColisionesTorpedos()
{
    // Detectar colisiones entre torpedos y obstáculos
    for (int i = m_torpedos.size() - 1; i >= 0; --i) {
        if (!m_torpedos[i].estaActivo()) {
            continue;
        }

        for (int j = m_obstaculos.size() - 1; j >= 0; --j) {
            bool colision = m_torpedos[i].hitbox().intersects(
                m_obstaculos[j].hitbox(),
                m_torpedos[i].position(),
                m_obstaculos[j].position()
                );

            if (colision) {
                // Reproducir sonido de explosión
                if (m_sonidoExplosion) {
                    m_sonidoExplosion->play();
                }

                // Destruir obstáculo y desactivar torpedo
                m_obstaculos.removeAt(j);
                m_torpedos[i].desactivar();
                break;
            }
        }
    }
}

void NivelIso::updateDificultad()
{
    // Ajustar dificultad según tiempo transcurrido
    qreal segundosTranscurridos = m_tiempoTranscurrido / 60.0;

    // Sistema de fases: cada 3-5 segundos aumenta la dificultad
    if (segundosTranscurridos < 5.0) {
        // Fase 1: FÁCIL
        m_frecuenciaGeneracion = 70;
        m_cantidadObstaculos = 2;
        m_scrollSpeedBase = 2.0;
        m_scrollSpeedSprint = 4.5;
    }
    else if (segundosTranscurridos < 8.0) {
        // Fase 2: NORMAL
        m_frecuenciaGeneracion = 55;
        m_cantidadObstaculos = 3;
        m_scrollSpeedBase = 2.3;
        m_scrollSpeedSprint = 5.0;
    }
    else if (segundosTranscurridos < 12.0) {
        // Fase 3: MEDIO
        m_frecuenciaGeneracion = 45;
        m_cantidadObstaculos = 3;
        m_scrollSpeedBase = 2.6;
        m_scrollSpeedSprint = 5.5;
    }
    else if (segundosTranscurridos < 15.0) {
        // Fase 4: DIFÍCIL
        m_frecuenciaGeneracion = 38;
        m_cantidadObstaculos = 4;
        m_scrollSpeedBase = 2.9;
        m_scrollSpeedSprint = 6.0;
    }
    else if (segundosTranscurridos < 18.0) {
        // Fase 5: MUY DIFÍCIL
        m_frecuenciaGeneracion = 32;
        m_cantidadObstaculos = 4;
        m_scrollSpeedBase = 3.2;
        m_scrollSpeedSprint = 6.5;
    }
    else {
        // Fase 6: EXTREMO
        m_frecuenciaGeneracion = 32;
        m_cantidadObstaculos = 4;
        m_scrollSpeedBase = 3.5;
        m_scrollSpeedSprint = 7.4;
    }

    // Aplicar velocidad según estado de sprint
    m_scrollSpeed = m_sprint ? m_scrollSpeedSprint : m_scrollSpeedBase;
}

void NivelIso::dibujarTiempo(QPainter &painter)
{
    painter.save();

    // Posición en esquina superior derecha
    int x = width() - 220;
    int y = 20;

    // Calcular tiempo restante
    int tiempoRestanteFrames = m_tiempoParaGanar - m_tiempoTranscurrido;
    int segundosRestantes = tiempoRestanteFrames / 60;
    int decimas = (tiempoRestanteFrames % 60) * 10 / 60;

    // Calcular tiempo transcurrido para dificultad
    int segundosTranscurridos = m_tiempoTranscurrido / 60;

    // Configurar fuente
    QFont font = painter.font();
    font.setPointSize(16);
    font.setBold(true);
    painter.setFont(font);

    // Texto del tiempo
    QString texto = QString("Tiempo: %1.%2s")
                        .arg(segundosRestantes, 2, 10, QChar('0'))
                        .arg(decimas);

    QRect textRect = painter.fontMetrics().boundingRect(texto);
    textRect.adjust(-15, -8, 15, 8);
    textRect.moveTo(x - 15, y - 8);

    // Fondo semi-transparente
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(textRect, 5, 5);

    // Color del texto según urgencia
    QColor colorTexto;
    if (segundosRestantes <= 5) {
        colorTexto = Qt::red;
    } else if (segundosRestantes <= 10) {
        colorTexto = Qt::yellow;
    } else {
        colorTexto = Qt::green;
    }

    painter.setPen(colorTexto);
    painter.drawText(x, y + 12, texto);

    // Barra de progreso del tiempo
    int barWidth = 180;
    int barHeight = 8;
    int barX = x;
    int barY = y + 25;

    // Fondo de la barra
    painter.setBrush(Qt::darkGray);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(barX, barY, barWidth, barHeight);

    // Progreso (se llena conforme pasa el tiempo)
    qreal progreso = qMin(1.0, static_cast<qreal>(m_tiempoTranscurrido) / m_tiempoParaGanar);
    int progressWidth = static_cast<int>(barWidth * progreso);

    // Color de barra según fase de dificultad
    QColor colorBarra;
    if (segundosTranscurridos < 8) {
        colorBarra = Qt::green;
    } else if (segundosTranscurridos < 15) {
        colorBarra = Qt::yellow;
    } else {
        colorBarra = Qt::red;
    }

    painter.setBrush(colorBarra);
    painter.setPen(Qt::NoPen);
    painter.drawRect(barX, barY, progressWidth, barHeight);

    // Indicador de nivel de dificultad
    QString nivelDificultad;
    QColor colorDificultad;

    if (segundosTranscurridos < 5) {
        nivelDificultad = "FÁCIL";
        colorDificultad = QColor(100, 255, 100);
    } else if (segundosTranscurridos < 8) {
        nivelDificultad = "NORMAL";
        colorDificultad = QColor(150, 255, 150);
    } else if (segundosTranscurridos < 12) {
        nivelDificultad = "MEDIO";
        colorDificultad = QColor(255, 255, 100);
    } else if (segundosTranscurridos < 15) {
        nivelDificultad = "DIFÍCIL";
        colorDificultad = QColor(255, 180, 50);
    } else if (segundosTranscurridos < 18) {
        nivelDificultad = "MUY DIFÍCIL";
        colorDificultad = QColor(255, 100, 50);
    } else {
        nivelDificultad = "EXTREMO";
        colorDificultad = Qt::red;
    }

    painter.setPen(colorDificultad);
    QFont dificultadFont = painter.font();
    dificultadFont.setPointSize(11);
    dificultadFont.setBold(true);
    painter.setFont(dificultadFont);
    painter.drawText(barX, barY + 20, nivelDificultad);

    // Indicador de sprint activo
    if (m_sprint) {
        painter.setPen(Qt::cyan);
        QFont sprintFont = painter.font();
        sprintFont.setPointSize(12);
        sprintFont.setBold(true);
        painter.setFont(sprintFont);
        painter.drawText(barX + 90, barY + 20, "SPRINT");
    }

    painter.restore();
}

void NivelIso::reiniciarNivel()
{
    m_timer->stop();

    // Reiniciar todas las variables de juego
    m_vidas = m_vidasMaximas;
    m_invulnerable = false;
    m_contadorInvulnerabilidad = 0;
    m_scrollOffset = 0.0;
    m_scrollSpeed = m_scrollSpeedBase;
    m_sprint = false;
    m_contadorFrames = 0;
    m_tiempoTranscurrido = 0;
    m_nivelCompletado = false;
    m_cooldownActual = 0;
    m_torpedos.clear();
    m_frecuenciaGeneracion = 60;
    m_cantidadObstaculos = 2;
    m_municionActual = m_municionMaxima;
    m_contadorRecarga = 0;

    // Reconfigurar escena
    initScene();

    m_timer->start(16);
}

void NivelIso::mostrarVictoria()
{
    m_timer->stop();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("¡Victoria!");
    msgBox.setText("¡Ganaste!");
    msgBox.setInformativeText(QString("Has sobrevivido 20 segundos.\n\n"
                                      "Vidas restantes: %1 / %2 \n\n"
                                      "¿Qué deseas hacer?")
                                  .arg(m_vidas)
                                  .arg(m_vidasMaximas));

    QPushButton *btnReiniciar = msgBox.addButton("Reiniciar Nivel", QMessageBox::ActionRole);
    QPushButton *btnMenu = msgBox.addButton("Volver al Menú", QMessageBox::ActionRole);

    msgBox.setDefaultButton(btnReiniciar);
    msgBox.exec();

    if (msgBox.clickedButton() == btnReiniciar) {
        reiniciarNivel();
    } else if (msgBox.clickedButton() == btnMenu) {
        emit volverAlMenu();
    }
}

void NivelIso::mostrarGameOver()
{
    m_timer->stop();

    qreal segundosSobrevividos = m_tiempoTranscurrido / 60.0;
    qreal segundosRestantes = (m_tiempoParaGanar - m_tiempoTranscurrido) / 60.0;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Game Over");
    msgBox.setText("¡Perdiste todas tus vidas!");

    msgBox.setInformativeText(
        QString("Te quedaste sin vidas.\n\n"
                "Estadísticas:\n"
                "Tiempo sobrevivido: %1 segundos\n"
                "Te faltaban: %2 segundos\n"
                "¿Qué deseas hacer?")
            .arg(segundosSobrevividos, 0, 'f', 1)
            .arg(segundosRestantes, 0, 'f', 1)
        );

    QPushButton *btnReiniciar = msgBox.addButton("Reintentar", QMessageBox::ActionRole);
    QPushButton *btnMenu = msgBox.addButton("Menú Principal", QMessageBox::ActionRole);

    msgBox.setDefaultButton(btnReiniciar);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    if (msgBox.clickedButton() == btnReiniciar) {
        reiniciarNivel();
    } else if (msgBox.clickedButton() == btnMenu) {
        emit volverAlMenu();
    }
}

void NivelIso::dibujarFondoScrolling(QPainter &painter)
{
    // Fondo base
    painter.fillRect(rect(), Qt::darkBlue);

    // Grid que simula movimiento
    painter.setPen(QColor(40, 60, 120, 100));

    int gridSize = 50;
    int offsetY = static_cast<int>(m_scrollOffset) % gridSize;

    // Líneas horizontales que se mueven hacia abajo
    for (int y = -gridSize + offsetY; y < height() + gridSize; y += gridSize) {
        painter.drawLine(0, y, width(), y);
    }

    // Líneas verticales (fijas)
    for (int x = 0; x < width(); x += gridSize) {
        painter.drawLine(x, 0, x, height());
    }
}

void NivelIso::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:
        m_moveLeft = true;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        m_moveRight = true;
        break;
    case Qt::Key_Up:
        m_sprint = true;
        m_scrollSpeed = m_scrollSpeedSprint;
        break;
    case Qt::Key_Space:
        dispararTorpedo();
        break;
    default:
        QWidget::keyPressEvent(event);
        break;
    }
}

void NivelIso::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:
        m_moveLeft = false;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        m_moveRight = false;
        break;
    case Qt::Key_Up:
        m_sprint = false;
        m_scrollSpeed = m_scrollSpeedBase;
        break;
    default:
        QWidget::keyReleaseEvent(event);
        break;
    }
}

void NivelIso::updateGame()
{
    // No actualizar si el nivel está completado
    if (m_nivelCompletado) {
        return;
    }

    // Incrementar tiempo transcurrido
    m_tiempoTranscurrido++;

    // Verificar condición de victoria
    if (m_tiempoTranscurrido >= m_tiempoParaGanar) {
        m_nivelCompletado = true;
        mostrarVictoria();
        return;
    }

    // Actualizar movimiento del barco según input
    updateBarcoFromInput();

    // Sistema de invulnerabilidad temporal tras recibir daño
    if (m_invulnerable) {
        m_contadorInvulnerabilidad--;
        if (m_contadorInvulnerabilidad <= 0) {
            m_invulnerable = false;
        }
    }

    // Actualizar cooldown de disparo
    if (m_cooldownActual > 0) {
        m_cooldownActual--;
    }

    // Sistema de recarga automática de munición
    if (m_contadorRecarga > 0) {
        m_contadorRecarga--;
        if (m_contadorRecarga == 0) {
            m_municionActual = m_municionMaxima;
        }
    }

    // Actualizar elementos del juego
    updateObstaculos();
    updateTorpedos();
    verificarColisionesTorpedos();
    generarNuevosObstaculos();
    updateDificultad();
    updateCollisions();

    // Actualizar offset del fondo (efecto de scrolling)
    m_scrollOffset += m_scrollSpeed;

    // Solicitar repintado
    update();
}

void NivelIso::updateBarcoFromInput()
{
    const qreal speed = 3.0;

    qreal dirY = 0.0;

    // Solo movimiento lateral (eje Y del mundo)
    if (m_moveLeft)
        dirY -= 1.0;

    if (m_moveRight)
        dirY += 1.0;

    QPointF posicionActual = m_barco.position();

    QPointF posicionNueva = posicionActual;
    posicionNueva.setY(posicionNueva.y() + dirY * speed);

    // Limitar movimiento al área jugable
    if (!m_playArea.isNull()) {
        if (posicionNueva.y() < m_playArea.top())
            posicionNueva.setY(m_playArea.top());
        if (posicionNueva.y() > m_playArea.bottom())
            posicionNueva.setY(m_playArea.bottom());
    }

    // Aplicar posición temporalmente
    m_barco.setPosition(posicionNueva);

    // Verificar colisión con obstáculos
    bool hayColision = false;
    for (const Obstaculon2 &o : m_obstaculos) {
        if (m_barco.hitbox().intersects(o.hitbox(),
                                        m_barco.position(),
                                        o.position())) {
            hayColision = true;
            break;
        }
    }

    // Si hay colisión, revertir movimiento
    if (hayColision) {
        m_barco.setPosition(posicionActual);
    }
}

void NivelIso::updateObstaculos()
{
    // Mover obstáculos hacia el barco (scrolling automático)
    for (int i = m_obstaculos.size() - 1; i >= 0; --i) {
        QPointF pos = m_obstaculos[i].position();
        pos.setX(pos.x() - m_scrollSpeed);
        m_obstaculos[i].setPosition(pos);

        // Eliminar obstáculos que pasaron el límite
        if (pos.x() < m_limiteEliminacion) {
            m_obstaculos.removeAt(i);
        }
    }
}

void NivelIso::generarNuevosObstaculos()
{
    m_contadorFrames++;

    // Generar obstáculos según frecuencia de dificultad
    if (m_contadorFrames >= m_frecuenciaGeneracion) {
        m_contadorFrames = 0;

        // Cantidad variable según dificultad
        int cantidadMin = qMax(1, m_cantidadObstaculos - 2);
        int cantidadMax = m_cantidadObstaculos;
        int cantidad = QRandomGenerator::global()->bounded(cantidadMin, cantidadMax + 1);

        for (int i = 0; i < cantidad; i++) {
            Obstaculon2 o;

            // Posición X: lejos (donde aparecen nuevos obstáculos)
            qreal x = m_limiteGeneracion;

            // Posición Y: aleatoria dentro del área jugable
            int minY = static_cast<int>(m_playArea.top() + 20.0);
            int maxY = static_cast<int>(m_playArea.bottom() - 20.0);
            qreal y = static_cast<qreal>(QRandomGenerator::global()->bounded(minY, maxY));

            o.setPosition(QPointF(x, y));
            m_obstaculos.append(o);
        }
    }
}

void NivelIso::updateCollisions()
{
    // Reiniciar estado de colisión
    m_barco.hitbox().setColliding(false);
    for (Obstaculon2 &o : m_obstaculos) {
        o.hitbox().setColliding(false);
    }

    // Solo verificar colisiones si no está invulnerable
    if (!m_invulnerable) {
        for (Obstaculon2 &o : m_obstaculos) {
            bool col = m_barco.hitbox().intersects(
                o.hitbox(),
                m_barco.position(),
                o.position());

            if (col) {
                m_barco.hitbox().setColliding(true);
                o.hitbox().setColliding(true);

                // Reproducir sonido de impacto
                if (m_sonidoExplosion) {
                    m_sonidoExplosion->play();
                }

                // Perder vida y activar invulnerabilidad
                m_vidas--;
                m_invulnerable = true;
                m_contadorInvulnerabilidad = 120;  // 2 segundos

                // Verificar game over
                if (m_vidas <= 0) {
                    m_nivelCompletado = true;
                    mostrarGameOver();
                    return;
                }

                break;
            }
        }
    } else {
        // Durante invulnerabilidad, solo marcar colisiones visualmente
        for (Obstaculon2 &o : m_obstaculos) {
            bool col = m_barco.hitbox().intersects(
                o.hitbox(),
                m_barco.position(),
                o.position());

            if (col) {
                m_barco.hitbox().setColliding(true);
                o.hitbox().setColliding(true);
            }
        }
    }
}

void NivelIso::drawHitbox(QPainter &painter,
                          const Hitbox &hitbox,
                          const QPointF &worldPos)
{
    // Obtener puntos de la hitbox en coordenadas del mundo
    QVector<QPointF> worldPoints = hitbox.worldPoints(worldPos);

    if (worldPoints.isEmpty())
        return;

    // Proyectar cada punto a pantalla isométrica
    QVector<QPointF> screenPoints;
    screenPoints.reserve(worldPoints.size());

    for (const QPointF &p : worldPoints) {
        screenPoints.append(ProyeccionIso::toScreen(p));
    }

    painter.save();
    // Verde = sin colisión, Rojo = colisión activa
    painter.setPen(hitbox.isColliding() ? Qt::red : Qt::green);
    painter.setBrush(Qt::NoBrush);

    QPolygonF poly(screenPoints);
    painter.drawPolygon(poly);

    painter.restore();
}
