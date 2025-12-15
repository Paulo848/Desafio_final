#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , nivelActual(nullptr)
    , nivelActualIso(nullptr)
    , nivelActual_(nullptr)
    , m_sonidoAmbiente(nullptr)
{
    ui->setupUi(this);

    // Configurar ventana
    setWindowTitle("Dia D");
    setMinimumSize(800, 600);

    ui->centralwidget->setStyleSheet(
        "QWidget#centralwidget {"
        "    background-image: url(:/ui/ui/fondo_menu.png);"
        "    background-position: center;"
        "    background-repeat: no-repeat;"
        "}"
        );

    QPalette palette;
    QPixmap fondoImagen(":/ui/ui/fondo_menu.png");

    // Escalar la imagen para llenar la ventana
    fondoImagen = fondoImagen.scaled(size(),
                                     Qt::KeepAspectRatioByExpanding,
                                     Qt::SmoothTransformation);

    palette.setBrush(QPalette::Window, QBrush(fondoImagen));
    ui->centralwidget->setPalette(palette);
    ui->centralwidget->setAutoFillBackground(true);

    // Genera un contenedor estatico
    contenedor = new QStackedWidget(this);

    // Agregar el menú como primera página
    contenedor->addWidget(ui->centralwidget);

    // Establecer el conteneder estatico como principal
    setCentralWidget(contenedor);

    // Mostrar el menú al inicio
    contenedor->setCurrentIndex(0);

    // Conectar botones del menú
    connect(ui->btnNivel1, &QPushButton::clicked, this, &MainWindow::cargarNivel1);
    connect(ui->btnNivel2, &QPushButton::clicked, this, &MainWindow::cargarNivel2);
    connect(ui->btnNivel3, &QPushButton::clicked, this, &MainWindow::cargarNivel3);

    // Cargar sonido del menú
    cargarSonidos();
}

MainWindow::~MainWindow()
{
    if (nivelActual != nullptr) {
        delete nivelActual;
    }
    if (nivelActualIso != nullptr) {
        delete nivelActualIso;
    }
    if (nivelActual_ != nullptr) {
        delete nivelActual_;
    }
    delete ui;

    // Limpia sonidos
    if (m_sonidoAmbiente) {
        m_sonidoAmbiente->stop();
        delete m_sonidoAmbiente;
        m_sonidoAmbiente = nullptr;
    }
}

void MainWindow::cargarNivel1()
{
    // Detener música del menú
    if (m_sonidoAmbiente) {
        disconnect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                   this, &MainWindow::onSonidoAmbienteTerminado);
        m_sonidoAmbiente->stop();
    }

    // Eliminar todos los niveles anteriores
    if (nivelActual != nullptr) {
        contenedor->removeWidget(nivelActual);
        delete nivelActual;
        nivelActual = nullptr;
    }
    if (nivelActualIso != nullptr) {
        contenedor->removeWidget(nivelActualIso);
        delete nivelActualIso;
        nivelActualIso = nullptr;
    }
    if (nivelActual_ != nullptr) {
        contenedor->removeWidget(nivelActual_);
        delete nivelActual_;
        nivelActual_ = nullptr;
    }

    // Crear nuevo nivel
    nivelActual_ = new Nivel_1(1, this);
    connect(nivelActual_, &Nivel_1::volverAlMenu, this, &MainWindow::mostrarMenuPrincipal);

    // Agregar nivel al contenedor
    contenedor->addWidget(nivelActual_);

    // Cambiar a la página del nivel
    contenedor->setCurrentWidget(nivelActual_);
}

void MainWindow::cargarNivel2()
{
    // Detener música del menú
    if (m_sonidoAmbiente) {
        disconnect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                   this, &MainWindow::onSonidoAmbienteTerminado);
        m_sonidoAmbiente->stop();
    }

    // Eliminar nivel anterior
    if (nivelActual != nullptr) {
        contenedor->removeWidget(nivelActual);
        delete nivelActual;
        nivelActual = nullptr;
    }
    if (nivelActualIso != nullptr) {
        contenedor->removeWidget(nivelActualIso);
        delete nivelActualIso;
        nivelActualIso = nullptr;
    }
    if (nivelActual_ != nullptr) {
        contenedor->removeWidget(nivelActual_);
        delete nivelActual_;
        nivelActual_ = nullptr;
    }

    // Crear el nivel isométrico
    nivelActualIso = new NivelIso(this);
    connect(nivelActualIso, &NivelIso::volverAlMenu, this, &MainWindow::mostrarMenuPrincipal);

    contenedor->addWidget(nivelActualIso);
    contenedor->setCurrentWidget(nivelActualIso);
}

void MainWindow::cargarNivel3()
{
    // Detener música del menú
    if (m_sonidoAmbiente) {
        disconnect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                   this, &MainWindow::onSonidoAmbienteTerminado);
        m_sonidoAmbiente->stop();
    }

    // Eliminar todos los niveles anteriores
    if (nivelActual != nullptr) {
        contenedor->removeWidget(nivelActual);
        delete nivelActual;
        nivelActual = nullptr;
    }
    if (nivelActualIso != nullptr) {
        contenedor->removeWidget(nivelActualIso);
        delete nivelActualIso;
        nivelActualIso = nullptr;
    }
    if (nivelActual_ != nullptr) {
        contenedor->removeWidget(nivelActual_);
        delete nivelActual_;
        nivelActual_ = nullptr;
    }

    // Crear nuevo nivel
    nivelActual = new Nivel(3, this);
    connect(nivelActual, &Nivel::volverAlMenu, this, &MainWindow::mostrarMenuPrincipal);

    // Agregar nivel al contenedor
    contenedor->addWidget(nivelActual);

    // Cambiar a la página del nivel
    contenedor->setCurrentWidget(nivelActual);
}

void MainWindow::mostrarMenuPrincipal()
{
    // Cambiar a la página del menú
    contenedor->setCurrentIndex(0);

    // Destruir nivel 2D
    if (nivelActual != nullptr) {
        contenedor->removeWidget(nivelActual);
        nivelActual->deleteLater();
        nivelActual = nullptr;
    }

    // Destruir nivel isométrico
    if (nivelActualIso != nullptr) {
        contenedor->removeWidget(nivelActualIso);
        nivelActualIso->deleteLater();
        nivelActualIso = nullptr;
    }

    // Destruir nivel 1
    if (nivelActual_ != nullptr) {
        contenedor->removeWidget(nivelActual_);
        nivelActual_->deleteLater();
        nivelActual_ = nullptr;
    }

    if (m_sonidoAmbiente) {
        connect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                this, &MainWindow::onSonidoAmbienteTerminado);

        if (!m_sonidoAmbiente->isPlaying()) {
            m_sonidoAmbiente->play();
        }
    }
}

// Sistema de sonido para el menú
void MainWindow::cargarSonidos()
{
    cargarSonidosDesdeArchivos();
}

void MainWindow::cargarSonidosDesdeArchivos()
{
    QString rutaBase = QCoreApplication::applicationDirPath();

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

    if (rutaEncontrada.isEmpty()) {
        return;
    }

    // Cargar sonido de ambiente del menú
    QString rutaAmbiente = rutaEncontrada + "/musica_menu.wav";
    m_sonidoAmbiente = new QSoundEffect(this);

    if (QFile::exists(rutaAmbiente)) {
        m_sonidoAmbiente->setSource(QUrl::fromLocalFile(rutaAmbiente));
        m_sonidoAmbiente->setVolume(0.15f);
        m_sonidoAmbiente->setLoopCount(1);

        connect(m_sonidoAmbiente, &QSoundEffect::playingChanged,
                this, &MainWindow::onSonidoAmbienteTerminado);

        // Iniciar reproducción
        m_sonidoAmbiente->play();

    } else {
        delete m_sonidoAmbiente;
        m_sonidoAmbiente = nullptr;
    }
}

void MainWindow::onSonidoAmbienteTerminado()
{
    if (m_sonidoAmbiente && !m_sonidoAmbiente->isPlaying()) {
        // Si el sonido terminó de reproducirse, reproducirlo nuevamente
        m_sonidoAmbiente->play();
    }
}
