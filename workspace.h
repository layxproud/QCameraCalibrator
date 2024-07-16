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
    void startThread(QThread *thread);
    void stopThread(QThread *thread);
    void ensureDirectoryIsClean(const QString &path);
    void clearDirectory(const QString &path);

signals:
    void frameReady(const cv::Mat &frame);
    void pointSelected(const QPointF &point);
    void calibrationFinished(bool success, const QString &message);
    void newConfiguration(const std::string &name);

public slots:
    void onPageChanged(int page);
    void onCaptureFrame();
    void onStartCalibration();

private:
    YamlHandler *yamlHandler;
    CameraThread *cameraThread;
    CalibrationThread *calibrationThread;
    MarkerThread *markerThread;

    int frameNumber;
    QString imagesDir;
};

#endif // WORKSPACE_H
