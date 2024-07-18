#ifndef CALIBRATIONTHREAD_H
#define CALIBRATIONTHREAD_H

#include "yamlhandler.h"
#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <QDir>
#include <QMutex>
#include <QThread>

class CalibrationThread : public QThread
{
    Q_OBJECT
public:
    explicit CalibrationThread(QObject *parent = nullptr);

    void setYamlHandler(YamlHandler *handler) { yamlHandler = handler; }
    void stop();

signals:
    void calibrationFinished(bool success, const QString &message);

protected:
    void run() override;

private:
    YamlHandler *yamlHandler;
    bool running;
    QMutex mutex;
    std::vector<cv::Mat> frames;
    cv::Ptr<cv::aruco::CharucoBoard> charucoBoard;
    cv::aruco::Dictionary dictionary;
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector;
};

#endif // CALIBRATIONTHREAD_H
