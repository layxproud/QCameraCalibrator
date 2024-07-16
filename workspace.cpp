#include "workspace.h"

Workspace::Workspace(QObject *parent)
    : QObject{parent}
    , yamlHandler(new YamlHandler(this))
    , cameraThread(new CameraThread(this))
    , calibrationThread(new CalibrationThread(this))
    , markerThread(new MarkerThread(this))
    , frameNumber(0)
    , imagesDir(QDir::currentPath() + "/images")
{
    connect(cameraThread, &CameraThread::frameReady, this, &Workspace::frameReady);
    connect(markerThread, &MarkerThread::frameReady, this, &Workspace::frameReady);
    connect(this, &Workspace::pointSelected, markerThread, &MarkerThread::onPointSelected);
    connect(markerThread, &MarkerThread::newConfiguration, this, &Workspace::newConfiguration);
    connect(
        calibrationThread,
        &CalibrationThread::calibrationFinished,
        this,
        &Workspace::calibrationFinished);
}

Workspace::~Workspace()
{
    stopThread(cameraThread);
    stopThread(calibrationThread);
    stopThread(markerThread);
}

void Workspace::init()
{
    calibrationThread->setYamlHandler(yamlHandler);
    markerThread->setYamlHandler(yamlHandler);

    startThread(cameraThread);
    ensureDirectoryIsClean(imagesDir);
}

void Workspace::startThread(QThread *thread)
{
    if (thread && !thread->isRunning()) {
        thread->start();
    }
}

void Workspace::stopThread(QThread *thread)
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

void Workspace::ensureDirectoryIsClean(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        clearDirectory(path);
    } else {
        dir.mkpath(".");
    }
}

void Workspace::onPageChanged(int page)
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

void Workspace::clearDirectory(const QString &path)
{
    QDir dir(path);
    dir.setNameFilters(QStringList() << "*.*");
    dir.setFilter(QDir::Files);
    foreach (QString dirFile, dir.entryList()) {
        dir.remove(dirFile);
    }
}

void Workspace::onCaptureFrame()
{
    cameraThread->saveCurrentFrame(imagesDir, frameNumber++);
}

void Workspace::onStartCalibration()
{
    startThread(calibrationThread);
}
