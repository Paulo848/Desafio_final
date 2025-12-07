#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "NivelIso.h"
#include "levels/nivel.h"
#include "nivel_1.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void cargarNivel1();
    void cargarNivel2();
    void cargarNivel3();
    void mostrarMenuPrincipal();

private:
    Ui::MainWindow *ui;
    QStackedWidget *contenedor;
    Nivel *nivelActual;
    NivelIso *nivelActualIso = nullptr;
    Nivel_1 *nivelActual_;
};

#endif // MAINWINDOW_H
