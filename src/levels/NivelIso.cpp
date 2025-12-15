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
    m_sonidoExplosion(nullptr),
    m_widgetOverlay(nullptr),
    m_contenedorResultado(nullptr),
    m_lblTitulo(nullptr),
    m_lblTiempo(nullptr),
    m_lblObstaculos(nullptr),
    m_lblVida(nullptr),
    m_btnReintentar(nullptr),
    m_btnMenu(nullptr),
    m_obstaculosDestruidos(0) // Decoracion menu
{
    setFocusPolicy(Qt::StrongFocus);

    // Cargar sonidos y configurar escena inicial
    cargarSonidos();
    cargarSpritesObstaculos();
    initScene();

    connect(m_timer, &QTimer::timeout, this, &NivelIso::updateGame);
    m_timer->start(16);

    // Botón para volver al menú principal
    QPushButton *btnVolver = new QPushButton("Volver", this);
    btnVolver->setGeometry(10, 10, 100, 30);
    connect(btnVolver, &QPushButton::clicked, this, [this]() {
        emit volverAlMenu();
    });
    crearOverlayResultado();
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
    // Barcoen posición inicial
    m_barco.setPosition(QPointF(-150.0, 0.0));

    // Limpiar obstáculos previos
    m_obstaculos.clear();

    // Generar algunos obstáculos iniciales alejados del barco
    for (int i = 0; i < 3; i++) {
        Obstaculon2 o;
        qreal x = 100.0 + i * 80.0;  // Lejos del barco
        qreal y = static_cast<qreal>(QRandomGenerator::global()->bounded(-100, 100));
        o.setPosition(QPointF(x, y));

        // --- asignar sprite aleatorio ---
        if (!m_spritesObstaculos.isEmpty()) {
            int n = m_spritesObstaculos.size();
            int id = QRandomGenerator::global()->bounded(n);
            o.setSpriteId(id);
        }

        m_obstaculos.append(o);
    }

    // Definir área jugable
    m_playArea = QRectF(-200.0, -150.0, 400.0, 300.0);
}

void NivelIso::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Color de fondo base (por si la imagen no carga)
    painter.fillRect(rect(), QColor(20, 30, 50));

    // Transformar al centro
    painter.translate(width() / 2.0, height() / 2.0);

    // Dibujar fondo con imagen de agua
    painter.save();
    painter.resetTransform();
    dibujarFondoScrolling(painter);
    painter.restore();

    // Resetear transformación para HUD
    painter.resetTransform();

    // Dibujar HUD (vidas, tiempo, munición)
    painter.save();
    dibujarVidas(painter);
    dibujarTiempo(painter);
    dibujarMunicion(painter);
    painter.restore();

    // Volver a centrar para elementos del juego
    painter.translate(width() / 2.0, height() / 2.0);

    // Dibujar marco del área jugable
    if (!m_playArea.isNull()) {
        QPointF tl = m_playArea.topLeft();
        QPointF tr = m_playArea.topRight();
        QPointF br = m_playArea.bottomRight();
        QPointF bl = m_playArea.bottomLeft();

        QVector<QPointF> pts;
        pts << ProyeccionIso::toScreen(tl)
            << ProyeccionIso::toScreen(tr)
            << ProyeccionIso::toScreen(br)
            << ProyeccionIso::toScreen(bl);

        painter.save();
        painter.setPen(QPen(QColor(255, 255, 255, 100), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(QPolygonF(pts));
        painter.restore();
    }

    // Dibujar barco
    QPointF barcoWorld = m_barco.position();
    QPointF barcoScreen = ProyeccionIso::toScreen(barcoWorld);

    bool dibujarBarco = true;
    if (m_invulnerable) {
        dibujarBarco = (m_contadorInvulnerabilidad % 10 < 5);
    }

    if (dibujarBarco) {
        painter.save();
        painter.translate(barcoScreen);

        if (!m_spriteBarco.isNull()) {
            int w = m_spriteBarco.width();
            int h = m_spriteBarco.height();
            painter.drawPixmap(-w / 2, -h / 2, m_spriteBarco);
        } else {
            painter.setBrush(Qt::yellow);
            painter.setPen(Qt::black);
            painter.drawRect(-BARCO_PROFUNDIDAD/2, -BARCO_ANCHO/2,
                             BARCO_PROFUNDIDAD, BARCO_ANCHO);
        }

        painter.restore();
    }

    // Dibujar obstáculos
    for (const Obstaculon2 &o : m_obstaculos) {
        QPointF oWorld  = o.position();
        QPointF oScreen = ProyeccionIso::toScreen(oWorld);

        painter.save();
        painter.translate(oScreen);

        if (!m_spritesObstaculos.isEmpty()) {
            int id = o.spriteId();
            if (id < 0 || id >= m_spritesObstaculos.size()) {
                id = 0;
            }

            const QPixmap &pix = m_spritesObstaculos[id];
            int w = pix.width();
            int h = pix.height();
            painter.drawPixmap(-w / 2, -h / 2, pix);
        } else {
            painter.setBrush(Qt::gray);
            painter.setPen(Qt::black);
            painter.drawRect(-OBS_PROFUNDIDAD/2, -OBS_ANCHO/2,
                             OBS_PROFUNDIDAD, OBS_ANCHO);
        }

        painter.restore();
    }

    // Dibujar torpedos
    for (const Torpedo &t : m_torpedos) {
        if (t.estaActivo()) {
            QPointF tWorld = t.position();
            QPointF tScreen = ProyeccionIso::toScreen(tWorld);

            painter.save();
            painter.translate(tScreen);

            if (t.tieneSprite() && !t.getSprite().isNull()) {
                const QPixmap &sprite = t.getSprite();
                int w = sprite.width();
                int h = sprite.height();
                painter.drawPixmap(-w / 2, -h / 2, sprite);
            }

            painter.restore();
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
            // Corazón lleno
            painter.setBrush(Qt::red);
            painter.setPen(Qt::darkRed);
        } else {
            // Corazón vacío
            painter.setBrush(Qt::darkGray);
            painter.setPen(Qt::gray);
        }

        painter.drawEllipse(corazonRect);
    }

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
    if (m_cooldownActual > 0 || m_municionActual <= 0) {
        return;
    }

    // Crear torpedo
    Torpedo torpedo;
    QPointF posBarco = m_barco.position();
    torpedo.setPosition(QPointF(posBarco.x() + 20, posBarco.y()));

    // Cargar sprite
    if (!m_spriteTorpedo.isNull()) {
        torpedo.setSprite(m_spriteTorpedo);
    }

    m_torpedos.append(torpedo);

    // Activar cooldown
    m_cooldownActual = m_cooldownDisparo;
    m_municionActual--;

    if (m_municionActual == 0) {
        m_contadorRecarga = m_tiempoRecarga;
    }

    if (m_sonidoDisparo) {
        m_sonidoDisparo->play();
    }
}

void NivelIso::updateTorpedos()
{
    // Actualizar posición y eliminar torpedos fuera del área
    for (int i = m_torpedos.size() - 1; i >= 0; --i) {
        // Solo actualizar si está activo
        if (m_torpedos[i].estaActivo()) {
            m_torpedos[i].actualizar();

            // Eliminar torpedos que salieron del mapa
            if (m_torpedos[i].position().x() > m_limiteGeneracion + 100) {
                m_torpedos.removeAt(i);
            }
        } else {
            m_torpedos.removeAt(i);
        }
    }
}

void NivelIso::verificarColisionesTorpedos()
{
    // Iterar en orden inverso para poder eliminar de forma segura
    for (int i = m_torpedos.size() - 1; i >= 0; --i) {
        // Verificar que el torpedo esté activo
        if (!m_torpedos[i].estaActivo()) {
            m_torpedos.removeAt(i);
            continue;
        }

        bool huboColision = false;

        // Verificar colisión con obstáculos
        for (int j = m_obstaculos.size() - 1; j >= 0; --j) {
            bool colision = m_torpedos[i].hitbox().intersects(
                m_obstaculos[j].hitbox(),
                m_torpedos[i].position(),
                m_obstaculos[j].position()
                );

            if (colision) {
                // Reproducir sonido
                if (m_sonidoExplosion) {
                    m_sonidoExplosion->play();
                }

                m_obstaculosDestruidos++;

                // Eliminar obstáculo
                m_obstaculos.removeAt(j);

                // Marcar colisión
                huboColision = true;

                // Salir del loop de obstáculos
                break;
            }
        }

        // Si hubo colisión, eliminar el torpedo
        if (huboColision) {
            m_torpedos.removeAt(i);
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

    // Fondo
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(textRect, 5, 5);

    // Color del texto
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

    // Progreso
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

    m_obstaculosDestruidos = 0;

    initScene();
    m_barcoPosPrevio = m_barco.position();

    m_timer->start(16);
}

void NivelIso::mostrarVictoria()
{
    m_timer->stop();
    mostrarOverlay(true);
}

void NivelIso::mostrarGameOver()
{
    m_timer->stop();
    mostrarOverlay(false);
}

void NivelIso::dibujarFondoScrolling(QPainter &painter)
{
    // Cargar la imagen de fondo agua
    if (m_spriteMapaIso.isNull()) {
        m_spriteMapaIso = QPixmap(":/fondos/nivel_2/fondo_agua.png");
    }

    if (!m_spriteMapaIso.isNull()) {
        const qreal parallaxFactor = 0.3;
        qreal offsetX = m_scrollOffset * parallaxFactor;

        int fondoWidth = m_spriteMapaIso.width();
        int fondoHeight = m_spriteMapaIso.height();

        // Escalar para llenar toda la pantalla verticalmente
        qreal scale = static_cast<qreal>(height()) / fondoHeight;
        int scaledWidth = static_cast<int>(fondoWidth * scale);
        int scaledHeight = height();

        int offsetXInt = static_cast<int>(offsetX) % scaledWidth;
        if (offsetXInt < 0) offsetXInt += scaledWidth;

        int numCopias = (width() / scaledWidth) + 3;

        for (int i = -1; i < numCopias; i++) {
            int x = i * scaledWidth - offsetXInt;
            QRectF target(x, 0, scaledWidth, scaledHeight);

            // Dibujar la imagen completa
            painter.drawPixmap(target, m_spriteMapaIso, m_spriteMapaIso.rect());
        }
    } else {
        painter.fillRect(rect(), QColor(0, 180, 200));
    }
}


void NivelIso::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        m_moveLeft = true;
        break;
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
    case Qt::Key_Left:
        m_moveLeft = false;
        break;
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

    // Actualizar offset del fondo
    m_scrollOffset += m_scrollSpeed;

    // Solicitar repintado
    update();
}

void NivelIso::updateBarcoFromInput()
{
    const qreal speed = 3.0;

    qreal dirY = 0.0;

    // Solo movimiento lateral
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
    // Mover obstáculos hacia el barco
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

            // asignar sprite aleatorio
            if (!m_spritesObstaculos.isEmpty()) {
                int n = m_spritesObstaculos.size();
                int id = QRandomGenerator::global()->bounded(n);
                o.setSpriteId(id);
            }

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
                m_contadorInvulnerabilidad = 120;

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
    painter.setPen(hitbox.isColliding() ? Qt::red : Qt::green);
    painter.setBrush(Qt::NoBrush);

    QPolygonF poly(screenPoints);
    painter.drawPolygon(poly);

    painter.restore();
}

void NivelIso::cargarSpritesObstaculos()
{
    m_spritesObstaculos.clear();

    // Fondo
    m_spriteMapaIso = QPixmap(":/obs/nivel_2/mapa_base.png");

    const qreal scaleFactorObs = 0.45;

    for (int i = 1; i <= 8; ++i) {
        QString ruta = QString(":/obs/nivel_2/obs_%1.png").arg(i);
        QPixmap sprite(ruta);

        if (!sprite.isNull()) {
            if (scaleFactorObs != 1.0) {
                int newW = sprite.width()  * scaleFactorObs;
                int newH = sprite.height() * scaleFactorObs;
                sprite = sprite.scaled(newW, newH,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
            }
            m_spritesObstaculos.append(sprite);
        }
    }

    m_spriteBarco = QPixmap(":/obs/nivel_2/barco.png");

    const qreal scaleFactorBarco = 0.3;
    if (!m_spriteBarco.isNull()) {

        int newW = int(m_spriteBarco.width()  * scaleFactorBarco);
        int newH = int(m_spriteBarco.height() * scaleFactorBarco);

        m_spriteBarco = m_spriteBarco.scaled(newW, newH,
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
    }

    m_spriteTorpedo = QPixmap(":/obs/nivel_2/torpedo.png");

    const qreal scaleFactorTorpedo = 0.07;  // Ajusta según necesites
    if (!m_spriteTorpedo.isNull()) {
        int newW = int(m_spriteTorpedo.width()  * scaleFactorTorpedo);
        int newH = int(m_spriteTorpedo.height() * scaleFactorTorpedo);

        m_spriteTorpedo = m_spriteTorpedo.scaled(newW, newH,
                                                 Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);
    }
}

void NivelIso::crearOverlayResultado()
{
    // Ventana que cubre todo
    m_widgetOverlay = new QWidget(this);
    m_widgetOverlay->setGeometry(0, 0, width(), height());
    m_widgetOverlay->setStyleSheet(
        "QWidget {"
        "    background-color: rgba(0, 0, 0, 180);"
        "}"
        );
    m_widgetOverlay->hide();

    // Caja Central
    m_contenedorResultado = new QWidget(m_widgetOverlay);
    m_contenedorResultado->setFixedSize(600, 400);
    m_contenedorResultado->setStyleSheet(
        "QWidget {"
        "    background-color: rgba(30, 35, 45, 240);"
        "    border-radius: 20px;"
        "    border: 3px solid rgba(255, 255, 255, 100);"
        "}"
        );

    // Layout del contenedor
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(m_contenedorResultado);
    layoutPrincipal->setSpacing(20);
    layoutPrincipal->setContentsMargins(40, 40, 40, 40);

    // Título
    m_lblTitulo = new QLabel("VICTORIA", m_contenedorResultado);
    m_lblTitulo->setAlignment(Qt::AlignCenter);
    m_lblTitulo->setStyleSheet(
        "QLabel {"
        "    color: #4FFFB0;"
        "    font-size: 48px;"
        "    font-weight: bold;"
        "    border: none;"
        "    background: transparent;"
        "}"
        );

    // Estadísticas
    QString estiloEstadistica =
        "QLabel {"
        "    color: #E0E0E0;"
        "    font-size: 18px;"
        "    border: none;"
        "    background: transparent;"
        "    padding: 5px;"
        "}";

    m_lblTiempo = new QLabel("Tiempo sobrevivido: 0.0s", m_contenedorResultado);
    m_lblTiempo->setAlignment(Qt::AlignCenter);
    m_lblTiempo->setStyleSheet(estiloEstadistica);

    m_lblObstaculos = new QLabel("Obstáculos destruidos: 0", m_contenedorResultado);
    m_lblObstaculos->setAlignment(Qt::AlignCenter);
    m_lblObstaculos->setStyleSheet(estiloEstadistica);

    m_lblVida = new QLabel("Vida final: 0 / 4", m_contenedorResultado);
    m_lblVida->setAlignment(Qt::AlignCenter);
    m_lblVida->setStyleSheet(estiloEstadistica);

    // Botones
    QString estiloBoton =
        "QPushButton {"
        "    background-color: rgba(70, 80, 100, 200);"
        "    color: white;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    border: 2px solid rgba(255, 255, 255, 100);"
        "    border-radius: 8px;"
        "    padding: 15px 30px;"
        "    min-width: 180px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(90, 100, 120, 220);"
        "    border: 2px solid rgba(255, 255, 255, 150);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(60, 70, 90, 200);"
        "}";

    m_btnReintentar = new QPushButton("Reintentar", m_contenedorResultado);
    m_btnReintentar->setStyleSheet(estiloBoton);
    m_btnReintentar->setCursor(Qt::PointingHandCursor);

    m_btnMenu = new QPushButton("Volver al Menú", m_contenedorResultado);
    m_btnMenu->setStyleSheet(estiloBoton);
    m_btnMenu->setCursor(Qt::PointingHandCursor);

    // Botones
    QHBoxLayout *layoutBotones = new QHBoxLayout();
    layoutBotones->setSpacing(20);
    layoutBotones->addStretch();
    layoutBotones->addWidget(m_btnReintentar);
    layoutBotones->addWidget(m_btnMenu);
    layoutBotones->addStretch();

    // Implementar
    layoutPrincipal->addWidget(m_lblTitulo);
    layoutPrincipal->addSpacing(20);
    layoutPrincipal->addWidget(m_lblTiempo);
    layoutPrincipal->addWidget(m_lblObstaculos);
    layoutPrincipal->addWidget(m_lblVida);
    layoutPrincipal->addStretch();
    layoutPrincipal->addLayout(layoutBotones);

    // Conectar botones
    connect(m_btnReintentar, &QPushButton::clicked, this, [this]() {
        ocultarOverlay();
        reiniciarNivel();
    });

    connect(m_btnMenu, &QPushButton::clicked, this, [this]() {
        ocultarOverlay();
        emit volverAlMenu();
    });
}

void NivelIso::mostrarOverlay(bool esVictoria)
{
    qreal tiempoSobrevivido = m_tiempoTranscurrido / 60.0;
    qreal tiempoTotal = m_tiempoParaGanar / 60.0;

    // Configurar título según resultado
    if (esVictoria) {
        m_lblTitulo->setText("¡VICTORIA!");
        m_lblTitulo->setStyleSheet(
            "QLabel {"
            "    color: #4FFFB0;"
            "    font-size: 48px;"
            "    font-weight: bold;"
            "    border: none;"
            "    background: transparent;"
            "}"
            );
    } else {
        m_lblTitulo->setText("DERROTA");
        m_lblTitulo->setStyleSheet(
            "QLabel {"
            "    color: #FF4F5B;"
            "    font-size: 48px;"
            "    font-weight: bold;"
            "    border: none;"
            "    background: transparent;"
            "}"
            );
    }

    // Actualizar estadísticas
    if (esVictoria) {
        m_lblTiempo->setText(QString("Tiempo sobrevivido: %1s / %2s")
                                 .arg(tiempoSobrevivido, 0, 'f', 1)
                                 .arg(tiempoTotal, 0, 'f', 1));

        m_lblObstaculos->setText(QString("Obstáculos destruidos: %1")
                                     .arg(m_obstaculosDestruidos));

        m_lblVida->setText(QString("Vida final: %1 / %2")
                               .arg(m_vidas)
                               .arg(m_vidasMaximas));
    } else {
        qreal tiempoRestante = tiempoTotal - tiempoSobrevivido;

        m_lblTiempo->setText(QString("Tiempo sobrevivido: %1s")
                                 .arg(tiempoSobrevivido, 0, 'f', 1));

        m_lblObstaculos->setText(QString("Te faltaban: %1s")
                                     .arg(tiempoRestante, 0, 'f', 1));

        m_lblVida->setText(QString("Obstáculos destruidos: %1")
                               .arg(m_obstaculosDestruidos));
    }

    // Ventana cubre todo
    m_widgetOverlay->setGeometry(0, 0, width(), height());

    // Centrar contenedor
    int x = (width() - m_contenedorResultado->width()) / 2;
    int y = (height() - m_contenedorResultado->height()) / 2;
    m_contenedorResultado->move(x, y);

    // Mostrar y traer al frente
    m_widgetOverlay->show();
    m_widgetOverlay->raise();
}

void NivelIso::ocultarOverlay()
{
    m_widgetOverlay->hide();
}
