#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , workspace(new Workspace(this))
{
    ui->setupUi(this);
    resize(960, 560);

    graphicsViewContainer = new GraphicsViewContainer(this);
    ui->cameraLayout->addWidget(graphicsViewContainer);

    ui->toolBox->setCurrentIndex(0);

    QShortcut *captuteFrameShortcut = new QShortcut(Qt::Key_Space, ui->captureButton);

    // Connects from GUI
    connect(ui->captureButton, &QPushButton::clicked, workspace, &Workspace::onCaptureFrame);
    QObject::connect(
        captuteFrameShortcut, &QShortcut::activated, workspace, &Workspace::onCaptureFrame);
    connect(ui->calibrateButton, &QPushButton::clicked, workspace, &Workspace::onStartCalibration);
    connect(ui->toolBox, &QToolBox::currentChanged, workspace, &Workspace::onPageChanged);
    connect(
        ui->markerSizeInput,
        QOverload<int>::of(&QSpinBox::valueChanged),
        workspace,
        &Workspace::onMarkerSizeChanged);
    connect(ui->saveConfigButton, &QPushButton::clicked, this, &MainWindow::onSaveConfiguration);
    connect(this, &MainWindow::pointSelected, workspace, &Workspace::pointSelected);
    connect(this, &MainWindow::saveConfiguration, workspace, &Workspace::saveConfiguration);

    // Connects to GUI
    connect(
        workspace,
        &Workspace::frameReady,
        graphicsViewContainer,
        &GraphicsViewContainer::updateFrame);
    connect(workspace, &Workspace::taskFinished, this, &MainWindow::onTaskFinished);
    connect(
        workspace,
        &Workspace::calibrationParamsMissing,
        this,
        &MainWindow::onCalibrationParametersMissing);
    connect(workspace, &Workspace::newConfiguration, this, &MainWindow::onNewConfiguration);

    workspace->init();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (graphicsViewContainer->geometry().contains(event->pos())) {
            QGraphicsView *view = graphicsViewContainer->getView();
            QPointF mappedPos = view->mapToScene(view->mapFromGlobal(event->globalPos()));
            emit pointSelected(mappedPos);
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::onTaskFinished(bool success, const QString &message)
{
    if (success) {
        QMessageBox::information(this, tr("Успех"), message);
    } else {
        QMessageBox::warning(this, tr("Ошибка"), message);
    }
}

void MainWindow::onNewConfiguration(const Configuration &config)
{
    ui->blockTypeInput->setText(QString::fromStdString(config.type));
    ui->blockNameInput->setText(QString::fromStdString(config.name));
}

void MainWindow::onCalibrationParametersMissing()
{
    ui->toolBox->setCurrentIndex(0);
}

void MainWindow::onSaveConfiguration()
{
    Configuration newConfiguration{};
    newConfiguration.name = ui->blockNameInput->text().toStdString();
    newConfiguration.type = ui->blockTypeInput->text().toStdString();
    emit saveConfiguration(newConfiguration);
}
