#ifndef YAMLHANDLER_H
#define YAMLHANDLER_H

#include <opencv2/opencv.hpp>
#include <QObject>

struct Configuration
{
    std::vector<int> markerIds;
    std::map<int, cv::Point3f> relativePoints;
    std::string name;
};

struct CalibrationParams
{
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
};

class YamlHandler : public QObject
{
    Q_OBJECT
public:
    YamlHandler(QObject *parent = nullptr);

    bool loadCalibrationParameters(const std::string &filename, CalibrationParams &params);
    bool loadConfigurations(
        const std::string &filename, std::map<std::string, Configuration> &configurations);
    bool saveConfigurations(
        const std::string &filename, const std::map<std::string, Configuration> &configurations);
};

#endif // YAMLHANDLER_H
