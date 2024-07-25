#include "markerthread.h"
#include <QDebug>

MarkerThread::MarkerThread(QObject *parent)
    : QThread{parent}
    , running(false)
{
    updateConfigurationsMap();

    AruCoDict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
    detectorParams = cv::aruco::DetectorParameters();
    detector = cv::aruco::ArucoDetector(AruCoDict, detectorParams);

    objPoints = cv::Mat(4, 1, CV_32FC3);
    objPoints.ptr<cv::Vec3f>(0)[0] = cv::Vec3f(-markerSize / 2.f, markerSize / 2.f, 0);
    objPoints.ptr<cv::Vec3f>(0)[1] = cv::Vec3f(markerSize / 2.f, markerSize / 2.f, 0);
    objPoints.ptr<cv::Vec3f>(0)[2] = cv::Vec3f(markerSize / 2.f, -markerSize / 2.f, 0);
    objPoints.ptr<cv::Vec3f>(0)[3] = cv::Vec3f(-markerSize / 2.f, -markerSize / 2.f, 0);
}

void MarkerThread::stop()
{
    QMutexLocker locker(&mutex);
    running = false;
}

void MarkerThread::run()
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

            markerIds.clear();
            markerPoints.clear();
            rvecs.clear();
            tvecs.clear();

            std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCorners;
            detector.detectMarkers(frame, markerCorners, markerIds, rejectedCorners);

            if (markerIds.size() > 0) {
                cv::aruco::drawDetectedMarkers(currentFrame, markerCorners, markerIds);

                int nMarkers = markerCorners.size();
                rvecs.resize(nMarkers);
                tvecs.resize(nMarkers);

                for (size_t i = 0; i < nMarkers; i++) {
                    solvePnP(
                        objPoints,
                        markerCorners.at(i),
                        calibrationParams.cameraMatrix,
                        calibrationParams.distCoeffs,
                        rvecs.at(i),
                        tvecs.at(i));

                    cv::Point2f markerCenter = (markerCorners.at(i)[0] + markerCorners.at(i)[1]
                                                + markerCorners.at(i)[2] + markerCorners.at(i)[3])
                                               * 0.25;
                    markerPoints.emplace_back(
                        markerCenter, cv::Point3f(tvecs[i][0], tvecs[i][1], tvecs[i][2]));
                }

                updateSelectedPointPosition();

                if (selectedPoint != cv::Point3f(0.0, 0.0, 0.0)
                    && !currentConfiguration.name.empty()) {
                    std::vector<cv::Point3f> points3D = {selectedPoint};
                    std::vector<cv::Point2f> points2D;
                    cv::projectPoints(
                        points3D,
                        cv::Vec3d::zeros(),
                        cv::Vec3d::zeros(),
                        calibrationParams.cameraMatrix,
                        calibrationParams.distCoeffs,
                        points2D);

                    cv::circle(currentFrame, points2D[0], 5, cv::Scalar(0, 0, 255), -1);
                }
            } else {
                detectCurrentConfiguration();
            }
            emit frameReady(currentFrame);
        }
        msleep(30);
    }

    cap.release();
}

void MarkerThread::onPointSelected(const QPointF &point)
{
    QMutexLocker locker(&mutex);

    if (markerIds.size() != 4) {
        emit taskFinished(false, tr("Для конфигурации блока необходимо ровно 4 маркера"));
        return;
    }

    cv::Point2f clickedPoint2D(point.x(), point.y());

    float depth = getDepthAtPoint(clickedPoint2D);
    cv::Point3f clickedPoint3D = projectPointTo3D(clickedPoint2D, depth);

    Configuration newConfig;
    newConfig.markerIds = markerIds;
    newConfig.name = currentConfiguration.name.empty() ? "New Configuration"
                                                       : currentConfiguration.name;

    for (int markerId : newConfig.markerIds) {
        auto it = std::find(markerIds.begin(), markerIds.end(), markerId);
        if (it != markerIds.end()) {
            int index = std::distance(markerIds.begin(), it);
            cv::Point3f relativePoint
                = calculateRelativePosition(clickedPoint3D, rvecs.at(index), tvecs.at(index));
            newConfig.relativePoints[markerId] = relativePoint;
        }
    }

    configurations[newConfig.name] = newConfig;
    // Костыль для того чтобы ГУИ успела обновиться
    // Выглядит ужасно но что поделать
    Configuration tempConfig = newConfig;
    detectCurrentConfiguration();
    if (tempConfig.name == currentConfiguration.name) {
        currentConfiguration = tempConfig;
    }
}

void MarkerThread::updateConfigurationsMap()
{
    //    configurations.clear();
    currentConfiguration.name = "...---..."; // Если была ошибка ввода то имя вернется к нормальному
    yamlHandler->loadConfigurations("configurations.yml", configurations);
}

void MarkerThread::detectCurrentConfiguration()
{
    Configuration new_Configuration;
    new_Configuration.name = "";
    for (const auto &config : configurations) {
        for (int id : config.second.markerIds) {
            if (std::find(markerIds.begin(), markerIds.end(), id) != markerIds.end()) {
                new_Configuration = config.second;
                break;
            }
        }
        if (!new_Configuration.name.empty()) {
            break;
        }
    }

    if (new_Configuration.name != currentConfiguration.name) {
        emit newConfiguration(new_Configuration.name);
        currentConfiguration = new_Configuration;
    }
}

float MarkerThread::getDepthAtPoint(const cv::Point2f &point)
{
    float minDist = std::numeric_limits<float>::max();
    float depth = 0.0f;

    for (const auto &marker : markerPoints) {
        float dist = cv::norm(point - marker.first);
        if (dist < minDist) {
            minDist = dist;
            depth = marker.second.z;
        }
    }
    return depth;
}

cv::Point3f MarkerThread::projectPointTo3D(const cv::Point2f &point2D, float depth)
{
    float x = (point2D.x - calibrationParams.cameraMatrix.at<double>(0, 2))
              / calibrationParams.cameraMatrix.at<double>(0, 0);
    float y = (point2D.y - calibrationParams.cameraMatrix.at<double>(1, 2))
              / calibrationParams.cameraMatrix.at<double>(1, 1);
    return cv::Point3f(x * depth, y * depth, depth);
}

cv::Point3f MarkerThread::calculateRelativePosition(
    const cv::Point3f &point3D, const cv::Vec3d &rvec, const cv::Vec3d &tvec)
{
    cv::Mat rotationMatrix;
    cv::Rodrigues(rvec, rotationMatrix);

    cv::Mat rotationMatrixInv = rotationMatrix.inv();
    cv::Mat tvecInv = -rotationMatrixInv * cv::Mat(tvec);

    cv::Mat pointMat = (cv::Mat_<double>(3, 1) << point3D.x, point3D.y, point3D.z);
    cv::Mat relativePointMat = rotationMatrixInv * pointMat + tvecInv;

    return cv::Point3f(
        relativePointMat.at<double>(0),
        relativePointMat.at<double>(1),
        relativePointMat.at<double>(2));
}

void MarkerThread::updateSelectedPointPosition()
{
    detectCurrentConfiguration();
    if (currentConfiguration.name.empty()) {
        return;
    }
    const auto &config = currentConfiguration;
    std::vector<cv::Point3f> allPoints;

    for (int id : config.markerIds) {
        auto it = std::find(markerIds.begin(), markerIds.end(), id);
        if (it != markerIds.end()) {
            int index = std::distance(markerIds.begin(), it);

            cv::Mat rotationMatrix;
            cv::Rodrigues(rvecs.at(index), rotationMatrix);

            cv::Mat relativePointMat
                = (cv::Mat_<double>(3, 1) << config.relativePoints.at(id).x,
                   config.relativePoints.at(id).y,
                   config.relativePoints.at(id).z);
            cv::Mat newPointMat = rotationMatrix * relativePointMat + cv::Mat(tvecs.at(index));

            allPoints.push_back(cv::Point3f(
                newPointMat.at<double>(0), newPointMat.at<double>(1), newPointMat.at<double>(2)));
        }
    }

    if (!allPoints.empty()) {
        cv::Point3f medianPoint = calculateMedianPoint(allPoints);
        selectedPoint = medianPoint;
    }
}

cv::Point3f MarkerThread::calculateMedianPoint(const std::vector<cv::Point3f> &points)
{
    std::vector<cv::Point3f> sortedPoints = points;
    std::sort(sortedPoints.begin(), sortedPoints.end(), [](const cv::Point3f &a, const cv::Point3f &b) {
        return a.x < b.x;
    });

    size_t medianIndex = points.size() / 2;
    return sortedPoints[medianIndex];
}
