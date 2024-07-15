#ifndef CALIBRATIONTHREAD_H
#define CALIBRATIONTHREAD_H

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

    void setFrames(const std::vector<cv::Mat> &f) { frames = f; }
    void stop();

signals:
    void calibrationFinished(bool success);

protected:
    void run() override;

private:
    bool running;
    QMutex mutex;
    std::vector<cv::Mat> frames;
    cv::Ptr<cv::aruco::CharucoBoard> charucoBoard;
    cv::aruco::Dictionary dictionary;
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector;

    // bool loadCalibrationParameters(const std::string &filename, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs);
    bool saveCalibrationParameters(
        const std::string &filename, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs);
};

#endif // CALIBRATIONTHREAD_H
