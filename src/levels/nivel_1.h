#ifndef NIVEL_1_H
#define NIVEL_1_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <QKeyEvent>
#include <QPushButton>
#include <vector>
#include "avion.h"
#include "oleadaat_colectivo.h"
#include <Qvector>
#include <QRandomGenerator>
#include <cstdlib>

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
    void update();
    void DisparosEnemigosAuto();

private:
    // Número del nivel
    int numNivel;

    // Sistema gráfico
    QGraphicsView *vista;
    QGraphicsScene *escena;
    QGraphicsPixmapItem *foto1;
    QGraphicsPixmapItem *foto2;
    int width;
    int speed;


    // Elementos del juego
    std::vector<Participantes*> participantes;
    std::vector<Obstaculo*> obstaculos;
    Avion* jugador;
    std::vector<Avion*> enemigos;
    bool HayIA = false;
    OleadaAt_Colectivo* IA;

    // UI
    QPushButton *btnVolver;
    QTimer* timer;
    QTimer* timer1;
    QTimer* tiempodesplazarelementos;

    // Métodos de inicialización
    void inicializarUI();
    void inicializarEscena();
    void cargarElementosNivel();

    // Lógica del juego
    void manejarColisiones(Avion* avion);
    bool actualizarIA();
    short int CantEnemigosIA = 3;
    void generarIA();
};


#endif // NIVEL_1_H
