#ifndef OBSTACULO_H
#define OBSTACULO_H

#include <QGraphicsItem>
#include <QPainter>
#include <QPixmap>
#include <QString>

enum class FormaObstaculo {
    Circulo,
    Rectangulo
};

class Obstaculo : public QGraphicsItem
{
private:
    FormaObstaculo forma;

    // Para círculos
    qreal radio;

    // Para rectángulos
    qreal ancho;
    qreal alto;

    // Bounding rect precalculado
    QRectF bounds;

    // --- Sprite opcional ---
    QPixmap sprite;      // imagen a dibujar
    bool    tieneSprite; // ¿hay sprite cargado?

public:
    // --- Constructor circular ---
    explicit Obstaculo(qreal _radio,
                       const QString &spritePath = QString());

    // --- Constructor rectangular ---
    explicit Obstaculo(qreal _ancho, qreal _alto,
                       const QString &spritePath = QString());

    // Bounding rect y shape para colisiones
    QRectF boundingRect() const override { return bounds; }
    QPainterPath shape() const override;

    // Dibujar obstáculo
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    // Forma
    inline FormaObstaculo getForma() const { return forma; }

    // Sprite (por si quieres cambiarlo luego)
    inline bool usaSprite() const { return tieneSprite; }
    void setSprite(const QString &spritePath);
};

#endif // OBSTACULO_H
