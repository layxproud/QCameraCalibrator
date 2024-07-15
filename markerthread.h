#ifndef MARKERTHREAD_H
#define MARKERTHREAD_H

#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <QMutex>
#include <QThread>

struct Configuration
{
    std::vector<int> markerIds;
    std::map<int, cv::Point3f> relativePoints;
    std::string name;
};

class MarkerThread : public QThread
{
    Q_OBJECT
public:
    explicit MarkerThread(QObject *parent = nullptr);
    void stop();

signals:
    void frameReady(const cv::Mat &frame);

protected:
    void run() override;

public slots:
    void onPointSelected(const QPointF &point);

private:
    bool running;
    cv::Mat currentFrame;
    cv::VideoCapture cap;
    QMutex mutex;

    float markerSize = 0.31f;

    cv::aruco::Dictionary AruCoDict;
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector;
    cv::Mat objPoints;

    std::map<std::string, Configuration> configurations;
    std::string currentConfigurationName;

    std::vector<int> markerIds;
    cv::Mat cameraMatrix, distCoeffs;
    std::vector<cv::Vec3d> rvecs;
    std::vector<cv::Vec3d> tvecs;
    std::vector<std::pair<cv::Point2f, cv::Point3f>> markerPoints;

    cv::Point3f selectedPoint;
    cv::Point3f relativeSelectedPoint;

    bool loadCalibrationParameters(const std::string &filename);
    bool loadConfigurations(const std::string &filename);
    bool saveConfigurations(const std::string &filename);
    void detectCurrentConfiguration();

    float getDepthAtPoint(const cv::Point2f &point);
    cv::Point3f projectTo3D(const cv::Point2f &point2D, float depth);
    cv::Point3f calculateRelativePosition(
        const cv::Point3f &point3D, const cv::Vec3d &rvec, const cv::Vec3d &tvec);
    void updateSelectedPointPosition();
};

#endif // MARKERTHREAD_H
