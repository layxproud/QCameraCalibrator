#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

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

    connect(ui->captureButton, &QPushButton::clicked, workspace, &Workspace::onCaptureFrame);
    connect(ui->calibrateButton, &QPushButton::clicked, workspace, &Workspace::onStartCalibration);
    connect(ui->toolBox, &QToolBox::currentChanged, workspace, &Workspace::onPageChanged);
    connect(
        ui->markerSizeInput,
        QOverload<int>::of(&QSpinBox::valueChanged),
        workspace,
        &Workspace::onMarkerSizeChanged);
    connect(ui->saveConfigButton, &QPushButton::clicked, this, &MainWindow::onSaveConfiguration);
    connect(this, &MainWindow::pointSelected, workspace, &Workspace::pointSelected);
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

void MainWindow::onNewConfiguration(const std::string &name)
{
    ui->configNameInput->setText(QString::fromStdString(name));
}

void MainWindow::onCalibrationParametersMissing()
{
    ui->toolBox->setCurrentIndex(0);
}

void MainWindow::onSaveConfiguration()
{
    workspace->saveConfiguration(ui->configNameInput->text());
}
