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
    , scene(new QGraphicsScene(this))
    , frameNumber(0)
    , imagesDir(QDir::currentPath() + "/images")
{
    ui->setupUi(this);

    ui->cameraView->setScene(scene);

    connect(ui->captureButton, &QPushButton::clicked, this, &MainWindow::onCaptureFrame);
    connect(ui->calibrateButton, &QPushButton::clicked, this, &MainWindow::onStartCalibration);
    connect(ui->toolBox, &QToolBox::currentChanged, this, &MainWindow::onPageChanged);

    connect(cameraThread, &CameraThread::frameReady, this, &MainWindow::updateFrame);
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

    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (ui->cameraView->geometry().contains(event->pos())) {
            QPointF mappedPos = ui->cameraView->mapToScene(event->pos());
            mappedPos -= QPointF(10, 10);
            qDebug() << mappedPos.x() << " , " << mappedPos.y();
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
    } else {
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

void MainWindow::updateFrame(const cv::Mat &frame)
{
    cv::Mat frameCopy = frame.clone();
    QImage img(frameCopy.data, frameCopy.cols, frameCopy.rows, frameCopy.step, QImage::Format_RGB888);
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(img.rgbSwapped()));
    scene->clear();
    scene->addItem(item);
    ui->cameraView->fitInView(item, Qt::KeepAspectRatio);

    //    cv::Mat rgbFrame;
    //    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
    //    QImage img((const uchar *) rgbFrame.data, rgbFrame.cols, rgbFrame.rows, QImage::Format_RGB888);
    //    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(img));
    //    scene->clear();
    //    scene->addItem(item);
    //    ui->cameraView->fitInView(item, Qt::KeepAspectRatio);
}
