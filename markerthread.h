#ifndef MARKERTHREAD_H
#define MARKERTHREAD_H

#include "yamlhandler.h"
#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <QMutex>
#include <QThread>

class MarkerThread : public QThread
{
    Q_OBJECT
public:
    explicit MarkerThread(QObject *parent = nullptr);
    void setYamlHandler(YamlHandler *handler) { yamlHandler = handler; }
    void setCalibrationParams(const CalibrationParams &params) { calibrationParams = params; }
    Configuration getCurrConfiguration() { return currentConfiguration; }
    void stop();

signals:
    void frameReady(const cv::Mat &frame);
    void newConfiguration(const std::string &name);

protected:
    void run() override;

public slots:
    void onPointSelected(const QPointF &point);
    void setMarkerSize(int size) { markerSize = (float) size / 100.0; }

private:
    bool running;
    cv::Mat currentFrame;
    cv::VideoCapture cap;
    QMutex mutex;
    YamlHandler *yamlHandler;

    float markerSize = 0.31f;
    cv::aruco::Dictionary AruCoDict;
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector;
    cv::Mat objPoints;

    Configuration currentConfiguration;
    std::map<std::string, Configuration> configurations;

    CalibrationParams calibrationParams;
    std::vector<int> markerIds;
    std::vector<cv::Vec3d> rvecs;
    std::vector<cv::Vec3d> tvecs;
    std::vector<std::pair<cv::Point2f, cv::Point3f>> markerPoints;

    cv::Point3f selectedPoint;

    void detectCurrentConfiguration();

    float getDepthAtPoint(const cv::Point2f &point);
    cv::Point3f projectPointTo3D(const cv::Point2f &point2D, float depth);
    cv::Point3f calculateRelativePosition(
        const cv::Point3f &point3D, const cv::Vec3d &rvec, const cv::Vec3d &tvec);
    void updateSelectedPointPosition();
};

#endif // MARKERTHREAD_H
