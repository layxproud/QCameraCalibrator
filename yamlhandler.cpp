#include "yamlhandler.h"

YamlHandler::YamlHandler(QObject *parent)
    : QObject(parent)
{}

bool YamlHandler::loadCalibrationParameters(const std::string &filename, CalibrationParams &params)
{
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened())
        return false;
    fs["CameraMatrix"] >> params.cameraMatrix;
    fs["DistCoeffs"] >> params.distCoeffs;
    fs.release();
    return true;
}

bool YamlHandler::loadConfigurations(
    const std::string &filename, std::map<std::string, Configuration> &configurations)
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
        configurations.insert(std::make_pair(config.name, config));
    }
    fs.release();
    return true;
}

bool YamlHandler::saveConfigurations(
    const std::string &filename, const std::map<std::string, Configuration> &configurations)
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
