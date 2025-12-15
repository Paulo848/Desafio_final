#include "obstaculo.h"

// =========================
//  Helper: cargar sprite
// =========================
static bool cargarSprite(const QString &path, QPixmap &out)
{
    if (path.isEmpty())
        return false;

    QPixmap tmp(path);
    if (tmp.isNull())
        return false;

    out = tmp;
    return true;
}

// =========================
//    CIRCULAR
// =========================
Obstaculo::Obstaculo(qreal _radio,
                     const QString &spritePath)
    : forma(FormaObstaculo::Circulo),
    radio(_radio),
    ancho(0),
    alto(0),
    bounds(-_radio, -_radio, 2*_radio, 2*_radio),
    sprite(),
    tieneSprite(false)
{
    setPos(0, 0); // Posición se ajusta externamente

    // Sprite opcional
    tieneSprite = cargarSprite(spritePath, sprite);
}


// =========================
//    RECTANGULAR
// =========================
Obstaculo::Obstaculo(qreal _ancho, qreal _alto,
                     const QString &spritePath)
    : forma(FormaObstaculo::Rectangulo),
    radio(0),
    ancho(_ancho),
    alto(_alto),
    bounds(-_ancho/2.0, -_alto/2.0, _ancho, _alto),
    sprite(),
    tieneSprite(false)
{
    setPos(0, 0);

    // Sprite opcional
    tieneSprite = cargarSprite(spritePath, sprite);
}


// =========================
//    SHAPE (colisión real)
// =========================
QPainterPath Obstaculo::shape() const
{
    QPainterPath path;
    if (forma == FormaObstaculo::Circulo)
        path.addEllipse(bounds);
    else
        path.addRect(bounds);

    return path;
}

// =========================
//       DIBUJO
// =========================
void Obstaculo::paint(QPainter *p,
                      const QStyleOptionGraphicsItem *,
                      QWidget *)
{
    // Si hay sprite → lo dibujamos dentro de bounds
    if (tieneSprite && !sprite.isNull()) {
        // target = bounds (en coords locales del item)
        QRectF target = bounds;
        QRectF source = sprite.rect(); // rect entero del pixmap
        p->drawPixmap(target, sprite, source);
        return;
    }

    // Si no hay sprite → debug simple
    p->setPen(Qt::NoPen);

    if (forma == FormaObstaculo::Circulo) {
        p->setBrush(QColor(Qt::red));
        p->drawEllipse(bounds);
    } else {
        p->setBrush(QColor(Qt::yellow));
        p->drawRect(bounds);
    }
}

// =========================
//   Cambiar sprite en runtime
// =========================
void Obstaculo::setSprite(const QString &spritePath)
{
    QPixmap nuevo;
    if (cargarSprite(spritePath, nuevo)) {
        sprite      = nuevo;
        tieneSprite = true;
        update();
    } else {
        tieneSprite = false;
        sprite      = QPixmap();
        update();
    }
}
