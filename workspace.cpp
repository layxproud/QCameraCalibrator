#include "workspace.h"
#include <QDebug>
#include <QFileDialog>

Workspace::Workspace(QObject *parent)
    : QObject{parent}
    , yamlHandler(new YamlHandler(this))
    , cameraThread(new CameraThread(this))
    , calibrationThread(new CalibrationThread(this))
    , markerThread(new MarkerThread(this))
    , frameNumber(0)
    , imagesDir(QDir::currentPath() + "/images")
    , calibrationStatus(false)
{
    connect(cameraThread, &CameraThread::frameReady, this, &Workspace::frameReady);
    connect(markerThread, &MarkerThread::frameReady, this, &Workspace::frameReady);
    connect(this, &Workspace::pointSelected, markerThread, &MarkerThread::onPointSelected);
    connect(markerThread, &MarkerThread::newConfiguration, this, &Workspace::newConfiguration);
    connect(
        this,
        &Workspace::configurationsUpdated,
        markerThread,
        &MarkerThread::updateConfigurationsMap);
    connect(calibrationThread, &CalibrationThread::taskFinished, this, &Workspace::taskFinished);
    connect(yamlHandler, &YamlHandler::taskFinished, this, &Workspace::taskFinished);
}

Workspace::~Workspace()
{
    stopThread(cameraThread);
    stopThread(calibrationThread);
    stopThread(markerThread);
}

void Workspace::init()
{
    // Load calibration params
    calibrationFileName = "calibration.yml";
    ensureDirectoryIsClean(imagesDir);
    calibrationStatus
        = yamlHandler->loadCalibrationParameters(calibrationFileName, calibrationParams);
    emit calibrationUpdated(calibrationStatus);

    // Initialize threads
    calibrationThread->setYamlHandler(yamlHandler);
    markerThread->setYamlHandler(yamlHandler);

    startThread(cameraThread);
}

std::map<std::string, Configuration> Workspace::getConfigurations()
{
    std::map<std::string, Configuration> configurations;
    yamlHandler->loadConfigurations("configurations.yml", configurations);
    return configurations;
}

void Workspace::onCaptureFrame()
{
    if (cameraThread->saveCurrentFrame(imagesDir, frameNumber++)) {
        emit frameCaptured(frameNumber);
    } else {
        frameNumber--;
        emit taskFinished(false, tr("Не удалось сделать снимок"));
    }
}

void Workspace::onStartCalibration()
{
    startThread(calibrationThread);
}

void Workspace::onMarkerSizeChanged(int size)
{
    markerThread->setMarkerSize(size);
}

void Workspace::saveConfiguration(const Configuration &newConfiguration)
{
    Configuration currentConfiguration = getCurrentConfiguration(newConfiguration);
    if (currentConfiguration.name == "") {
        emit taskFinished(false, tr("Блок не обнаружен"));
        return;
    }
    yamlHandler->updateConfigurations("configurations.yml", currentConfiguration);
    emit configurationsUpdated();
    return;
}

void Workspace::saveSingleConfiguration(
    const Configuration &newConfiguration, const QString &fileName)
{
    Configuration currentConfiguration = getCurrentConfiguration(newConfiguration);
    if (currentConfiguration.name == "") {
        emit taskFinished(false, tr("Блок не обнаружен"));
        return;
    }
    std::map<std::string, Configuration> configurations;
    configurations.insert(std::make_pair(currentConfiguration.name, currentConfiguration));
    if (yamlHandler->saveConfigurations(fileName.toStdString(), configurations)) {
        emit taskFinished(true, tr("Блок успешно сохранен в файл %1").arg(fileName));
    } else {
        emit taskFinished(false, tr("Произошла ошибка при сохранении блока!"));
    }
    return;
}

void Workspace::editConfiguration(const Configuration &newConfiguration)
{
    yamlHandler->updateConfigurations("configurations.yml", newConfiguration);
    emit configurationsUpdated();
}

void Workspace::removeConfiguration(const Configuration &config)
{
    yamlHandler->removeConfiguration("configurations.yml", config);
    emit configurationsUpdated();
}

void Workspace::exportConfiguration(const QString &fileName)
{
    std::map<std::string, Configuration> configurations;
    yamlHandler->loadConfigurations(fileName.toStdString(), configurations);
    for (const auto &config : configurations) {
        yamlHandler->updateConfigurations("configurations.yml", config.second);
    }
    emit configurationsUpdated();
}

void Workspace::selectCalibrationFile(const QString &fileName)
{
    if (yamlHandler->loadCalibrationParameters(fileName.toStdString(), calibrationParams)) {
        emit calibrationUpdated(true);
        calibrationFileName = fileName.toStdString();
    } else {
        emit calibrationUpdated(false);
        emit taskFinished(
            false, QString(tr("Could not load calibration params from file: %1").arg(fileName)));
    }
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

        // default
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
        CalibrationParams calibrationParams;
        if (!yamlHandler->loadCalibrationParameters(calibrationFileName, calibrationParams)) {
            emit calibrationParamsMissing();
            emit taskFinished(false, tr("Не обнаружен файл калибровки. Сначала откалибруйте камеру!"));
            return;
        }

        stopThread(cameraThread);
        stopThread(calibrationThread);
        startThread(markerThread);
        markerThread->setCalibrationParams(calibrationParams);
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

Configuration Workspace::getCurrentConfiguration(const Configuration &newConfiguration)
{
    // Получаю текущую конфигурацию из треда маркеров,
    // потому что информация о маркерах и положении цента известна только там
    Configuration currentConfiguration = markerThread->getCurrConfiguration();
    currentConfiguration.id = newConfiguration.id;
    currentConfiguration.type = newConfiguration.type;
    currentConfiguration.name = newConfiguration.name;
    currentConfiguration.date = newConfiguration.date;
    return currentConfiguration;
}
