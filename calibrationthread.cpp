#include "calibrationthread.h"
#include <opencv2/aruco/charuco.hpp>
#include <QDebug>

CalibrationThread::CalibrationThread(QObject *parent)
    : QThread(parent)
    , running(false)
{
    int squaresX = 7, squaresY = 5;
    float squareLength = 0.04f, markerLength = 0.02f;
    dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
    detectorParams = cv::aruco::DetectorParameters();
    detector = cv::aruco::ArucoDetector(dictionary, detectorParams);
    charucoBoard = new cv::aruco::CharucoBoard(
        cv::Size(squaresX, squaresY), squareLength, markerLength, dictionary);
}

void CalibrationThread::stop()
{
    QMutexLocker locker(&mutex);
    running = false;
}

void CalibrationThread::run()
{
    running = true;

    // Проверяем наличие изображений в папке
    QString imagesDir = QDir::currentPath() + "/images";
    QDir dir(imagesDir);
    QStringList filters;
    filters << "*.png"
            << "*.jpg"
            << "*.jpeg";
    dir.setNameFilters(filters);
    QStringList fileNames = dir.entryList();

    if (fileNames.isEmpty()) {
        qDebug() << "В директории " << imagesDir << " не обнаружены изображения";
        emit calibrationFinished(false);
        return;
    }

    // Преобразуем изображения в cv::Mat
    for (const QString &fileName : fileNames) {
        {
            QMutexLocker locker(&mutex);
            if (!running)
                return;
        }

        QString filePath = imagesDir + "/" + fileName;
        cv::Mat frame = cv::imread(filePath.toStdString());
        if (!frame.empty()) {
            frames.push_back(frame);
        }
    }

    if (frames.empty()) {
        qCritical() << "Не удалось преобразовать изображения в cv::Mat";
        emit calibrationFinished(false);
        return;
    }

    std::vector<std::vector<cv::Point2f>> allCorners;
    std::vector<std::vector<int>> allIds;

    for (const auto &frame : frames) {
        {
            QMutexLocker locker(&mutex);
            if (!running)
                return;
        }

        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners, corners_rejected;
        detector.detectMarkers(frame, corners, ids, corners_rejected);

        if (!ids.empty()) {
            std::vector<cv::Point2f> charucoCorners;
            std::vector<int> charucoIds;
            cv::aruco::interpolateCornersCharuco(
                corners, ids, frame, charucoBoard, charucoCorners, charucoIds);

            if (charucoCorners.size() >= 4) {
                allCorners.push_back(charucoCorners);
                allIds.push_back(charucoIds);
            }
        }
    }

    {
        QMutexLocker locker(&mutex);
        if (!running)
            return;
    }

    if (allCorners.empty()) {
        qCritical() << "Недостаточно данных для калибровки";
        emit calibrationFinished(false);
        return;
    }

    try {
        cv::Mat cameraMatrix, distCoeffs;
        std::vector<cv::Mat> rvecs, tvecs;
        double rms = cv::aruco::calibrateCameraCharuco(
            allCorners,
            allIds,
            charucoBoard,
            frames[0].size(),
            cameraMatrix,
            distCoeffs,
            rvecs,
            tvecs);

        if (rms > 0) {
            qDebug() << "Калибровка завершена c показателем RMS =" << rms;
            if (saveCalibrationParameters("calibration.yml", cameraMatrix, distCoeffs)) {
                qDebug() << "Файл калибровки сохранен";
                emit calibrationFinished(true);
            } else {
                qCritical() << "Не удалось сохранить файл калибровки";
                emit calibrationFinished(false);
            }
        } else {
            qCritical() << "Калибровка не удалась";
            emit calibrationFinished(false);
        }
    } catch (const cv::Exception &e) {
        qCritical() << "Ошибка калибровки: " << e.what();
        emit calibrationFinished(false);
    }
}

bool CalibrationThread::saveCalibrationParameters(
    const std::string &filename, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs)
{
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (!fs.isOpened())
        return false;
    fs << "CameraMatrix" << cameraMatrix;
    fs << "DistCoeffs" << distCoeffs;
    fs.release();
    return true;
}
