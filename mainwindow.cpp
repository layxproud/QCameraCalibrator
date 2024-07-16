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

    connect(ui->captureButton, &QPushButton::clicked, workspace, &Workspace::onCaptureFrame);
    connect(ui->calibrateButton, &QPushButton::clicked, workspace, &Workspace::onStartCalibration);
    connect(ui->toolBox, &QToolBox::currentChanged, workspace, &Workspace::onPageChanged);
    connect(this, &MainWindow::pointSelected, workspace, &Workspace::pointSelected);
    connect(
        workspace,
        &Workspace::frameReady,
        graphicsViewContainer,
        &GraphicsViewContainer::updateFrame);
    connect(workspace, &Workspace::calibrationFinished, this, &MainWindow::onCalibrationFinished);
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

void MainWindow::onCalibrationFinished(bool success, const QString &message)
{
    if (success) {
        QMessageBox::information(this, tr("Успешная калибровка"), message);
    } else {
        QMessageBox::warning(this, tr("Ошибка калибровки"), message);
    }
}

void MainWindow::onNewConfiguration(const std::string &name)
{
    ui->nameInput->setText(QString::fromStdString(name));
}
