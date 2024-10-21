#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <opencv2/opencv.hpp>
#include <QMutex>
#include <QThread>

class CameraThread : public QThread
{
    Q_OBJECT
public:
    explicit CameraThread(QObject *parent = nullptr);
    void stop();
    bool saveCurrentFrame(const QString &directory, int frameNumber);

signals:
    void frameReady(const cv::Mat &frame);

protected:
    void run() override;

private:
    bool running;
    cv::Mat currentFrame;
    cv::VideoCapture cap;
    QMutex mutex;
};

#endif // CAMERATHREAD_H
