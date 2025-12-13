#ifndef NIVELISO_H
#define NIVELISO_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QSoundEffect>
#include <QTimer>
#include <QPixmap>
#include "barco.h"
#include "obstaculon2.h"
#include "torpedo.h"

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
    // Constantes de renderizado
    static constexpr int BARCO_PROFUNDIDAD = 40;
    static constexpr int BARCO_ANCHO = 20;
    static constexpr int OBS_PROFUNDIDAD = 40;
    static constexpr int OBS_ANCHO = 20;

    void initScene();
    void updateBarcoFromInput();
    void updateCollisions();
    void drawHitbox(QPainter &painter, const Hitbox &hitbox, const QPointF &worldPos);
    void updateObstaculos();
    void generarNuevosObstaculos();
    void dibujarFondoScrolling(QPainter &painter);
    void dibujarVidas(QPainter &painter);
    void dibujarTiempo(QPainter &painter);
    void dibujarMunicion(QPainter &painter);
    void reiniciarNivel();
    void mostrarVictoria();
    void mostrarGameOver();
    void dispararTorpedo();
    void updateTorpedos();
    void verificarColisionesTorpedos();
    void updateDificultad();
    void cargarSonidos();
    void cargarSonidosDesdeArchivos();

    void cargarSpritesObstaculos();

    Barco m_barco;
    QVector<Obstaculon2> m_obstaculos;
    QVector<Torpedo> m_torpedos;
    QTimer *m_timer;

    bool m_moveLeft;
    bool m_moveRight;
    bool m_sprint;

    QRectF m_playArea;

    qreal m_scrollOffset;
    qreal m_scrollSpeed;
    qreal m_scrollSpeedBase;
    qreal m_scrollSpeedSprint;
    qreal m_limiteEliminacion;
    qreal m_limiteGeneracion;
    int m_contadorFrames;

    int m_vidas;
    int m_vidasMaximas;
    bool m_invulnerable;
    int m_contadorInvulnerabilidad;

    int m_tiempoTranscurrido;
    int m_tiempoParaGanar;
    bool m_nivelCompletado;

    int m_cooldownDisparo;
    int m_cooldownActual;

    int m_municionActual;
    int m_municionMaxima;
    int m_contadorRecarga;
    int m_tiempoRecarga;

    int m_frecuenciaGeneracion;
    int m_cantidadObstaculos;

    QSoundEffect *m_sonidoDisparo;
    QSoundEffect *m_sonidoExplosion;

    QVector<QPixmap> m_spritesObstaculos;

    QPixmap m_spriteBarco;
    QPixmap m_spriteTorpedo;
    QPixmap m_spriteMapaIso;
};

#endif // NIVELISO_H
