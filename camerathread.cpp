#include "camerathread.h"
#include <QDebug>

CameraThread::CameraThread(QObject *parent)
    : QThread(parent)
    , running(false)
{}

void CameraThread::stop()
{
    QMutexLocker locker(&mutex);
    running = false;
}

void CameraThread::run()
{
    cv::Mat resizedFrame;
    cv::Size newSize(640, 480);

    cap.open(0); // for webcam
    // cap.open(
    //     "udpsrc port=5000 caps = \"application/x-rtp, media=(string)video, clock-rate=(int)90000, "
    //     "encoding-name=(string)H264, payload=(int)96\" ! rtph264depay ! decodebin ! videoconvert ! "
    //     "appsink",
    //     cv::CAP_GSTREAMER);

    if (!cap.isOpened()) {
        qCritical() << "Failed to open video feed. Exiting thread.";
        return;
    }

    running = true;

    while (running) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
            continue;

        {
            QMutexLocker locker(&mutex);
            currentFrame = frame.clone();
            cv::resize(currentFrame, resizedFrame, newSize);
        }

        emit frameReady(resizedFrame);
    }

    cap.release();
}

bool CameraThread::saveCurrentFrame(const QString &directory, int frameNumber)
{
    QMutexLocker locker(&mutex);
    if (currentFrame.empty())
        return false;

    cv::Mat resizedFrame;
    cv::Size newSize(640, 480);
    cv::resize(currentFrame, resizedFrame, newSize);

    QString filePath = directory + QString("/frame_%1.png").arg(frameNumber, 3, 10, QChar('0'));
    return cv::imwrite(filePath.toStdString(), resizedFrame);
}
