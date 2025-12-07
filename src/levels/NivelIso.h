#ifndef NIVELISO_H
#define NIVELISO_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include "barco.h"
#include "obstaculon2.h"

class QTimer;

class NivelIso : public QWidget
{
    Q_OBJECT

public:
    explicit NivelIso(QWidget *parent = nullptr);
    ~NivelIso() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void updateGame();

signals:
    void volverAlMenu();

private:
    void initScene();
    void updateBarcoFromInput();
    void updateCollisions();
    void drawHitbox(QPainter &painter,
                    const Hitbox &hitbox,
                    const QPointF &worldPos);
    QPoint static toScreen();

    Barco m_barco;
    QVector<Obstaculon2> m_obstaculos;
    QTimer *m_timer;

    // Input simple
    bool m_moveLeft;
    bool m_moveRight;
    bool m_moveUp;
    bool m_moveDown;

    QRectF m_playArea;
};

#endif // NIVELISO_H
