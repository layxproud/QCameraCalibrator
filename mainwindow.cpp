#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , cameraThread(new CameraThread(this))
    , calibrationThread(new CalibrationThread(this))
    , markerThread(new MarkerThread(this))
    , frameNumber(0)
    , imagesDir(QDir::currentPath() + "/images")
{
    ui->setupUi(this);

    graphicsViewContainer = new GraphicsViewContainer(this);
    ui->cameraLayout->addWidget(graphicsViewContainer);

    connect(ui->captureButton, &QPushButton::clicked, this, &MainWindow::onCaptureFrame);
    connect(ui->calibrateButton, &QPushButton::clicked, this, &MainWindow::onStartCalibration);
    connect(ui->toolBox, &QToolBox::currentChanged, this, &MainWindow::onPageChanged);

    connect(
        cameraThread,
        &CameraThread::frameReady,
        graphicsViewContainer,
        &GraphicsViewContainer::updateFrame);
    connect(
        markerThread,
        &MarkerThread::frameReady,
        graphicsViewContainer,
        &GraphicsViewContainer::updateFrame);
    connect(this, &MainWindow::pointSelected, markerThread, &MarkerThread::onPointSelected);
    connect(
        calibrationThread,
        &CalibrationThread::calibrationFinished,
        this,
        &MainWindow::onCalibrationFinished);

    startThread(cameraThread);

    ensureDirectoryIsClean(imagesDir);
}

MainWindow::~MainWindow()
{
    stopThread(cameraThread);
    stopThread(calibrationThread);
    stopThread(markerThread);

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

void MainWindow::startThread(QThread *thread)
{
    if (thread && !thread->isRunning()) {
        thread->start();
    }
}

void MainWindow::stopThread(QThread *thread)
{
    if (thread && thread->isRunning()) {
        CameraThread *camera = dynamic_cast<CameraThread *>(thread);
        if (camera) {
            camera->stop();
            camera->wait();
            return;
        }

        CalibrationThread *calibration = dynamic_cast<CalibrationThread *>(thread);
        if (calibration) {
            calibration->stop();
            calibration->wait();
            return;
        }

        MarkerThread *marker = dynamic_cast<MarkerThread *>(thread);
        if (marker) {
            marker->stop();
            marker->wait();
            return;
        }

        thread->quit();
        thread->wait();
    }
}

void MainWindow::clearDirectory(const QString &path)
{
    QDir dir(path);
    dir.setNameFilters(QStringList() << "*.*");
    dir.setFilter(QDir::Files);
    foreach (QString dirFile, dir.entryList()) {
        dir.remove(dirFile);
    }
}

void MainWindow::ensureDirectoryIsClean(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        clearDirectory(path);
    } else {
        dir.mkpath(".");
    }
}

void MainWindow::onPageChanged(int page)
{
    if (page == 0) {
        stopThread(markerThread);
        startThread(cameraThread);
    } else {
        stopThread(cameraThread);
        stopThread(calibrationThread);
        startThread(markerThread);
    }
}

void MainWindow::onCaptureFrame()
{
    cameraThread->saveCurrentFrame(imagesDir, frameNumber++);
}

void MainWindow::onStartCalibration()
{
    startThread(calibrationThread);
}

void MainWindow::onCalibrationFinished(bool success)
{
    if (success) {
        qDebug() << "Калибровка завершена";
    } else {
        qWarning() << "Не удалось провести калибровку";
    }
}
