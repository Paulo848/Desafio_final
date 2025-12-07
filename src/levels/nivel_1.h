#ifndef NIVEL_1_H
#define NIVEL_1_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <QKeyEvent>
#include <QPushButton>
#include <vector>

class Participantes;
class Obstaculo;
class Agente;

class Nivel_1: public QWidget
{
    Q_OBJECT

public:
    explicit Nivel_1(int numeroNivel, QWidget *parent = nullptr);
    ~Nivel_1();

signals:
    void volverAlMenu();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void actualizarJuego(); // El timer
    void onVolverClicked();

private:
    // Número del nivel
    int numNivel;

    // Sistema gráfico
    QGraphicsView *vista;
    QGraphicsScene *escena;
    QGraphicsPixmapItem *bg1;
    QGraphicsPixmapItem *bg2;
    int width;
    int speed;


    // Elementos del juego
    std::vector<Participantes*> participantes;
    std::vector<Obstaculo*> obstaculos;
    std::vector<QRectF*> aviones;
    std::vector<QPolygonF*> barcos;
    Participantes *jugador;
    Agente *ia;

    // UI
    QPushButton *btnVolver;
    QTimer* timer;

    // Métodos de inicialización
    void inicializarUI();
    void inicializarEscena();
    void cargarElementosNivel();

    // Lógica del juego
    void manejarColisiones();
    void actualizarIA();

private slots:
    void update();

};

#endif // NIVEL_1_H
