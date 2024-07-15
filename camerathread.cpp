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
    cap.open(0);

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
        }

        emit frameReady(currentFrame);
        msleep(30);
    }

    cap.release();
}

void CameraThread::saveCurrentFrame(const QString &directory, int frameNumber)
{
    QMutexLocker locker(&mutex);
    if (currentFrame.empty())
        return;

    QString filePath = directory + QString("/frame_%1.png").arg(frameNumber, 3, 10, QChar('0'));
    cv::imwrite(filePath.toStdString(), currentFrame);
    qDebug() << "Frame captured and saved to" << filePath;
}
