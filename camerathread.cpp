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

    // cap.open(0);  // for webcam
    cap.open("rtsp://admin:QulonCamera1@192.168.1.85:554/video");

    if (!cap.isOpened())
        return;

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

void CameraThread::saveCurrentFrame(const QString &directory, int frameNumber)
{
    QMutexLocker locker(&mutex);
    if (currentFrame.empty())
        return;

    cv::Mat resizedFrame;
    cv::Size newSize(640, 480);
    cv::resize(currentFrame, resizedFrame, newSize);

    QString filePath = directory + QString("/frame_%1.png").arg(frameNumber, 3, 10, QChar('0'));
    cv::imwrite(filePath.toStdString(), resizedFrame);
    qDebug() << "Frame captured and saved to" << filePath;
}
