#include "NivelIso.h"

#include <QtMath>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QRectF>
#include <QPushButton>

#include "proyeccioniso.h"

NivelIso::NivelIso(QWidget *parent)
    : QWidget(parent),
    m_timer(new QTimer(this)),
    m_moveLeft(false),
    m_moveRight(false),
    m_moveUp(false),
    m_moveDown(false)
{
    // Este nivel necesita recibir eventos de teclado directamente
    setFocusPolicy(Qt::StrongFocus);

    // Configura el estado inicial del mundo lógico (barco, obstáculos, área jugable)
    initScene();

    // Bucle del juego: cada timeout dispara updateGame()
    connect(m_timer, &QTimer::timeout,
            this, &NivelIso::updateGame);

    // Aprox. 60 FPS (1000 ms / 16 ≈ 62.5)
    m_timer->start(16);

    // Botón para volver al menú principal (señal personalizada volverAlMenu)
    QPushButton *btnVolver = new QPushButton("Volver", this);
    btnVolver->setGeometry(10, 10, 100, 30);
    connect(btnVolver, &QPushButton::clicked, this, [this]() {
        emit volverAlMenu();
    });

}

NivelIso::~NivelIso()
{
}

// Qt usará este tamaño como mínimo aceptable
// (normalmente no permitirá hacer la ventana más pequeña que esto)
QSize NivelIso::minimumSizeHint() const
{ return QSize(640, 480); }

// Qt usará este tamaño como "ideal" al crear o ajustar la ventana
// si ningún layout externo impone otras restricciones
QSize NivelIso::sizeHint() const
{ return QSize(800, 600); }

/*
 * initScene configura el estado inicial del nivel:
 * - Define el tamaño del área jugable (m_playArea) en coordenadas de mundo.
 * - Posiciona el barco en el origen.
 * - Crea algunos obstáculos de prueba.
 */
void NivelIso::initScene()
{
    // Dimensiones del campo de juego en el plano lógico (mundo 2D):
    // x = "profundidad" (fondo → frente)
    // y = "ancho"       (izquierda ↔ derecha)
    qreal profundidad = 500.0;
    qreal ancho = 300.0;
    qreal posicionx = -profundidad/2.0;
    qreal posiciony = -ancho/2.0;

    // Posición inicial del barco (centro del área jugable)
    m_barco.setPosition(QPointF(0.0, 0.0));

    // Limpiamos y creamos obstáculos de prueba
    m_obstaculos.clear();

    // Obstáculo ejemplo 1
    Obstaculon2 o1;
    o1.setPosition(QPointF(150.0, 0.0));
    m_obstaculos.append(o1);

    // Obstáculo ejemplo 2
    Obstaculon2 o2;
    o2.setPosition(QPointF(-100.0, 50.0));
    m_obstaculos.append(o2);

    // Obstáculo ejemplo 3
    Obstaculon2 o3;
    o3.setPosition(QPointF(50.0, 120.0));
    m_obstaculos.append(o3);

    /*
     * m_playArea es el rectángulo que define el espacio jugable en el mundo 2D.
     * - El barco no puede salir de este rectángulo.
     * - Ajustando estas dimensiones cambiamos el "tamaño" del nivel.
     * - Si es demasiado grande, partes del área pueden quedar fuera de la ventana.
     *
     * Convención de ejes:
     *     x
     *     ^
     *    /
     *   /
     *  /
     *  -------> y
     */
    m_playArea = QRectF(posicionx, posiciony, profundidad, ancho);
}

/*
 * paintEvent construye cada fotograma del nivel:
 * - pinta fondo
 * - proyecta y dibuja el área jugable
 * - proyecta y dibuja barco y obstáculos
 * - dibuja las hitboxes de depuración
 */
void NivelIso::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Fondo simple (mar nocturno provisional)
    painter.fillRect(rect(), Qt::darkBlue);

    /*
     * Transformación base:
     * - Colocamos el origen del sistema de coordenadas en el centro de la ventana.
     * - A partir de aquí, el mundo 2D se dibuja "alrededor" del centro.
     */
    painter.translate(width() / 2.0, height() / 2.0);

    // Dibujar marco de la zona jugable (mundo 2D → pantalla isométrica)
    if (!m_playArea.isNull()) {
        // Esquinas del rectángulo en coordenadas de mundo
        QPointF tl = m_playArea.topLeft();
        QPointF tr = m_playArea.topRight();
        QPointF br = m_playArea.bottomRight();
        QPointF bl = m_playArea.bottomLeft();

        // Proyección isométrica de las 4 esquinas
        QVector<QPointF> pts;
        pts << ProyeccionIso::toScreen(tl)
            << ProyeccionIso::toScreen(tr)
            << ProyeccionIso::toScreen(br)
            << ProyeccionIso::toScreen(bl);

        painter.save();
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(QPolygonF(pts)); // marco blanco del área jugable
        painter.restore();
    }

    // Dibujar barco (rectángulo centrado en la posición proyectada del barco)
    int bar_profundidad = 40;                  // tamaño en "x pantalla"
    int bar_ancho = 20;                        // tamaño en "y pantalla"
    int bar_posicionx = -bar_profundidad/2;    // centrado en origen local
    int bar_posiciony = -bar_ancho/2;

    QPointF barcoWorld = m_barco.position();                 // posición en mundo
    QPointF barcoScreen = ProyeccionIso::toScreen(barcoWorld); // posición proyectada en pantalla

    painter.save();
    // Origen local del painter = posición del barco en pantalla
    painter.translate(barcoScreen);
    painter.setBrush(Qt::yellow);
    painter.setPen(Qt::black);
    painter.drawRect(bar_posicionx, bar_posiciony, bar_profundidad, bar_ancho);
    painter.restore();

    // Dibujar obstáculos (mismo patrón que el barco: centro → translate → rectángulo local)
    for (const Obstaculon2 &o : m_obstaculos) {
        QPointF oWorld = o.position();
        QPointF oScreen = ProyeccionIso::toScreen(oWorld);

        int obs_profundidad = 40;
        int obs_ancho = 20;
        int obs_posicionx = -obs_profundidad/2;
        int ob_posiciony = -obs_ancho/2;

        painter.save();
        painter.translate(oScreen);
        painter.setBrush(Qt::gray);
        painter.setPen(Qt::black);
        painter.drawRect(obs_posicionx, ob_posiciony, obs_profundidad, obs_ancho);
        painter.restore();
    }

    // Dibujar hitboxes (verde si no colisiona, rojo si está en colisión)
    drawHitbox(painter, m_barco.hitbox(), m_barco.position());

    for (const Obstaculon2 &o : m_obstaculos) {
        drawHitbox(painter, o.hitbox(), o.position());
    }
}

void NivelIso::keyPressEvent(QKeyEvent *event)
{
    // El teclado solo modifica flags de intención de movimiento;
    // el desplazamiento real se hace en updateBarcoFromInput().
    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:
        m_moveLeft = true;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        m_moveRight = true;
        break;
    case Qt::Key_W:
    case Qt::Key_Up:
        m_moveUp = true;
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        m_moveDown = true;
        break;
    default:
        QWidget::keyPressEvent(event);
        break;
    }
}

void NivelIso::keyReleaseEvent(QKeyEvent *event)
{
    // Al soltar la tecla, desactivamos la dirección correspondiente
    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_Left:
        m_moveLeft = false;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        m_moveRight = false;
        break;
    case Qt::Key_W:
    case Qt::Key_Up:
        m_moveUp = false;
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        m_moveDown = false;
        break;
    default:
        QWidget::keyReleaseEvent(event);
        break;
    }
}

void NivelIso::updateGame()
{
    // TODO futuro: si los obstáculos se mueven, aquí se llamaría a su update

    // 1) Actualizar la posición del barco según el input acumulado
    updateBarcoFromInput();

    // 2) Solo revisamos colisiones si el jugador se está moviendo
    if (m_moveLeft || m_moveRight || m_moveUp || m_moveDown)
        updateCollisions();

    // 3) Pedir repintado → desencadena una nueva llamada a paintEvent()
    update();
}

void NivelIso::updateBarcoFromInput()
{
    const qreal speed = 3.0; // velocidad base del barco en el plano de mundo

    QPointF dir(0.0, 0.0);

    // W: desplazamiento hacia "adelante" en profundidad (+x en el mundo)
    if (m_moveUp)
        dir += QPointF(1.0, 0.0);

    // S: desplazamiento hacia "atrás" en profundidad (−x en el mundo)
    if (m_moveDown)
        dir += QPointF(-1.0, 0.0);

    // A / D: desplazamiento lateral en el eje Y del mundo
    // (ajustado para que en pantalla se vea consistente con la proyección)
    if (m_moveLeft)
        dir += QPointF(0.0, -1.0);  // izquierda en pantalla

    if (m_moveRight)
        dir += QPointF(0.0, +1.0);  // derecha en pantalla

    // Normalizar para que en diagonal no se mueva más rápido
    if (!qFuzzyIsNull(dir.x()) || !qFuzzyIsNull(dir.y())) {
        qreal len = qSqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (len > 0.0)
            dir /= len;
    }

    // Nueva posición propuesta
    QPointF pos = m_barco.position();
    pos += dir * speed;

    // Limitar el movimiento al área jugable m_playArea
    if (!m_playArea.isNull()) {
        if (pos.x() < m_playArea.left())
            pos.setX(m_playArea.left());
        if (pos.x() > m_playArea.right())
            pos.setX(m_playArea.right());
        if (pos.y() < m_playArea.top())
            pos.setY(m_playArea.top());
        if (pos.y() > m_playArea.bottom())
            pos.setY(m_playArea.bottom());
    }

    m_barco.setPosition(pos);
}

void NivelIso::updateCollisions()
{
    // Resetear estado de colisión en todas las hitboxes
    m_barco.hitbox().setColliding(false);
    for (Obstaculon2 &o : m_obstaculos) {
        o.hitbox().setColliding(false);
    }

    // Revisar barco vs cada obstáculo (colisión por hitbox en mundo 2D)
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

void NivelIso::drawHitbox(QPainter &painter,
                          const Hitbox &hitbox,
                          const QPointF &worldPos)
{
    // Coordenadas de la hitbox en el mundo 2D (antes de proyectar)
    QVector<QPointF> worldPoints = hitbox.worldPoints(worldPos);

    if (worldPoints.isEmpty())
        return;

    // Convertir cada punto de la hitbox a coordenadas de pantalla isométrica
    QVector<QPointF> screenPoints;
    screenPoints.reserve(worldPoints.size());

    for (const QPointF &p : worldPoints) {
        screenPoints.append(ProyeccionIso::toScreen(p));
    }

    painter.save();
    // Verde si no colisiona, rojo si la hitbox está marcada en colisión
    painter.setPen(hitbox.isColliding() ? Qt::red : Qt::green);
    painter.setBrush(Qt::NoBrush);

    // Dibujar el polígono de la hitbox proyectada
    QPolygonF poly(screenPoints);
    painter.drawPolygon(poly);

    painter.restore();
}
