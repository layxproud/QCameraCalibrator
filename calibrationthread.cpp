#include "calibrationthread.h"
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <QDebug>

CalibrationThread::CalibrationThread(QObject *parent)
    : QThread(parent)
    , running(false)
{
    int squaresX = 7, squaresY = 5;
    float squareLength = 40.0f, markerLength = 20.0f;
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

    QString imagesDir = QDir::currentPath() + "/images";
    QDir dir(imagesDir);
    QStringList filters;
    filters << "*.png"
            << "*.jpg"
            << "*.jpeg";
    dir.setNameFilters(filters);
    QStringList fileNames = dir.entryList();

    if (fileNames.isEmpty()) {
        emit taskFinished(false, QString(tr("No images in directory %1")).arg(imagesDir));
        return;
    }

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
        } else {
            qWarning() << "Could not load image: " << fileName;
        }
    }

    if (frames.empty()) {
        emit taskFinished(false, tr("Could not transform images to cv::Mat"));
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
        emit taskFinished(false, tr("Not enough data to begin calibration"));
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
            if (yamlHandler->saveCalibrationParameters("calibration.yml", cameraMatrix, distCoeffs)) {
                emit taskFinished(
                    true, QString(tr("Calibration completed successfully with RMS = %1")).arg(rms));
            } else {
                emit taskFinished(
                    false, tr("Error occured while saving calibration parameters to file"));
            }
        } else {
            emit taskFinished(false, QString(tr("Calibration failed with RMS = %1")).arg(rms));
        }
    } catch (const cv::Exception &e) {
        emit taskFinished(false, QString(tr("Calibration error: %1")).arg(e.what()));
    }
}
