#ifndef NIVEL_1_H
#define NIVEL_1_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
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
#include <QCheckBox>
#include <QProgressBar>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QSoundEffect>

class Participantes;
class Obstaculo;
class Agente;

class Nivel_1: public QWidget
{
    Q_OBJECT

public:
    explicit Nivel_1(int numeroNivel, QWidget *parent = nullptr);
    ~Nivel_1();
    bool m_moverizquierda = false;
    bool m_moverarriba = false;
    bool m_moverderecha = false;
    bool m_moverabajo = false;

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
    void onSonidoAmbienteTerminado();

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
    Avion* jugador;
    short int derribados = 0;
    short int limit_derribados = 5;
    std::vector<Avion*> enemigos;
    bool HayIA = false;
    OleadaAt_Colectivo* IA;

    // UI
    QWidget* hudWidget;
    QPushButton *btnVolver;
    QTimer* timer;
    QTimer* timer1;
    QTimer* tiempodisparos;
    QProgressBar* barraVida;
    QProgressBar* barraOleadas;
    QLabel* lblRonda;
    QLabel* lblDanio;
    QLabel* lblEnemigos;
    QLabel* lblOleadas;

    // Métodos de inicialización
    void inicializarUI();
    void inicializarHUD();
    void actualizarHUD();
    void inicializarEscena();
    void cargarElementosNivel();

    // Sonido
    QSoundEffect *m_sonidoAmbiente;
    void cargarSonidos();
    void cargarSonidosDesdeArchivos();

    // Lógica del juego
    short int OleadasSuperadas = 0;
    short int TotalOleadas = 0;
    short int RondasSuperadas = 0;
    void manejarColisiones(Avion* avion);
    bool actualizarIA();
    short int CantEnemigosIA = 3;
    void generarIA();
    bool generarEnemigos();

    //Finalizacíon Juego.
    void MostrarResultadosdeJuego();
    void reiniciarNivel();
};


#endif // NIVEL_1_H
