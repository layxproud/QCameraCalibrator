#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "calibrationthread.h"
#include "markerthread.h"
#include "yamlhandler.h"
#include <camerathread.h>
#include <opencv2/opencv.hpp>
#include <QDir>
#include <QObject>

class Workspace : public QObject
{
    Q_OBJECT
public:
    explicit Workspace(QObject *parent = nullptr);
    ~Workspace();

    void init();
    std::map<std::string, Configuration> getConfigurations();

signals:
    void frameReady(const cv::Mat &frame);
    void pointSelected(const QPointF &point);
    void newConfiguration(const Configuration &config);
    void taskFinished(bool success, const QString &message);
    void calibrationParamsMissing();
    void configurationsUpdated();
    void calibrationUpdated(bool status);
    void frameCaptured(int num);

public slots:
    void onPageChanged(int page);
    void onCaptureFrame();
    void onStartCalibration();
    void onMarkerSizeChanged(int size);
    void saveConfiguration(const Configuration &newConfiguration);
    void saveSingleConfiguration(const Configuration &newConfiguration, const QString &fileName);
    void editConfiguration(const Configuration &newConfiguration);
    void removeConfiguration(const Configuration &config);
    void exportConfiguration(const QString &fileName);
    void selectCalibrationFile(const QString &fileName);

private:
    YamlHandler *yamlHandler;
    CameraThread *cameraThread;
    CalibrationThread *calibrationThread;
    MarkerThread *markerThread;

    int frameNumber;
    QString imagesDir;

    CalibrationParams calibrationParams;
    bool calibrationStatus;
    std::string calibrationFileName;

    void startThread(QThread *thread);
    void stopThread(QThread *thread);
    void ensureDirectoryIsClean(const QString &path);
    void clearDirectory(const QString &path);
    Configuration getCurrentConfiguration(const Configuration &newConfiguration);
};

#endif // WORKSPACE_H
