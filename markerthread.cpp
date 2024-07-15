#include "markerthread.h"
#include <QDebug>

MarkerThread::MarkerThread(QObject *parent)
    : QThread{parent}
    , running(false)
{
    loadCalibrationParameters("calibration.yml");
    loadConfigurations("configurations.yml");

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
                        cameraMatrix,
                        distCoeffs,
                        rvecs.at(i),
                        tvecs.at(i));

                    // Store the 2D and corresponding 3D points
                    cv::Point2f markerCenter = (markerCorners.at(i)[0] + markerCorners.at(i)[1]
                                                + markerCorners.at(i)[2] + markerCorners.at(i)[3])
                                               * 0.25;
                    markerPoints.emplace_back(
                        markerCenter, cv::Point3f(tvecs[i][0], tvecs[i][1], tvecs[i][2]));
                }

                updateSelectedPointPosition();
            }

            if (selectedPoint != cv::Point3f(0.0, 0.0, 0.0)) {
                std::vector<cv::Point3f> points3D = {selectedPoint};
                std::vector<cv::Point2f> points2D;
                cv::projectPoints(
                    points3D,
                    cv::Vec3d::zeros(),
                    cv::Vec3d::zeros(),
                    cameraMatrix,
                    distCoeffs,
                    points2D);

                cv::circle(currentFrame, points2D[0], 5, cv::Scalar(0, 0, 255), -1);
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
        qDebug() << "Для конфигурации блока необходимо ровно 4 маркера";
        return;
    }

    cv::Point2f clickedPoint2D(point.x(), point.y());

    float depth = getDepthAtPoint(clickedPoint2D);
    cv::Point3f clickedPoint3D = projectTo3D(clickedPoint2D, depth);

    Configuration newConfig;
    newConfig.markerIds = markerIds;
    newConfig.name = "My Configuration 2";

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
}

bool MarkerThread::loadCalibrationParameters(const std::string &filename)
{
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened())
        return false;
    fs["CameraMatrix"] >> cameraMatrix;
    fs["DistCoeffs"] >> distCoeffs;
    fs.release();
    return true;
}

bool MarkerThread::loadConfigurations(const std::string &filename)
{
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened())
        return false;

    cv::FileNode configsNode = fs["Configurations"];
    for (const auto &configNode : configsNode) {
        Configuration config;
        configNode["Name"] >> config.name;
        configNode["MarkerIds"] >> config.markerIds;
        cv::FileNode relativePointsNode = configNode["RelativePoints"];
        for (const auto &relativePointNode : relativePointsNode) {
            int markerId = std::stoi(relativePointNode.name().substr(7));
            relativePointNode >> config.relativePoints[markerId];
        }
        configurations[config.name] = config;
    }
    fs.release();
    return true;
}

bool MarkerThread::saveConfigurations(const std::string &filename)
{
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (!fs.isOpened())
        return false;

    fs << "Configurations"
       << "[";
    for (const auto &config : configurations) {
        fs << "{";
        fs << "Name" << config.second.name;
        fs << "MarkerIds"
           << "[";
        for (int id : config.second.markerIds) {
            fs << id;
        }
        fs << "]";
        fs << "RelativePoints"
           << "{";
        for (const auto &relativePoint : config.second.relativePoints) {
            fs << "Marker_" + std::to_string(relativePoint.first) << relativePoint.second;
        }
        fs << "}";
        fs << "}";
    }
    fs << "]";
    fs.release();
    return true;
}

void MarkerThread::detectCurrentConfiguration()
{
    for (const auto &config : configurations) {
        bool match = true;
        for (int id : config.second.markerIds) {
            if (std::find(markerIds.begin(), markerIds.end(), id) == markerIds.end()) {
                match = false;
                break;
            }
        }
        if (match) {
            currentConfigurationName = config.first;
            break;
        }
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

cv::Point3f MarkerThread::projectTo3D(const cv::Point2f &point2D, float depth)
{
    float x = (point2D.x - cameraMatrix.at<double>(0, 2)) / cameraMatrix.at<double>(0, 0);
    float y = (point2D.y - cameraMatrix.at<double>(1, 2)) / cameraMatrix.at<double>(1, 1);
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
    if (currentConfigurationName.empty())
        return;

    const auto &config = configurations[currentConfigurationName];
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

            selectedPoint = cv::Point3f(
                newPointMat.at<double>(0), newPointMat.at<double>(1), newPointMat.at<double>(2));
            break;
        }
    }
}
