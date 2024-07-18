#include "yamlhandler.h"
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <QDebug>

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

bool YamlHandler::saveCalibrationParameters(
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

bool YamlHandler::updateConfigurations(
    const std::string &filename, const Configuration &currentConfiguration)
{
    std::map<std::string, Configuration> existingConfigurations;
    loadConfigurations(filename, existingConfigurations);

    std::string duplicateName;
    ConflictType conflict
        = findDuplicateConfiguration(existingConfigurations, currentConfiguration, duplicateName);

    switch (conflict) {
    case ConflictType::None:
        existingConfigurations.insert(
            std::make_pair(currentConfiguration.name, currentConfiguration));
        break;
    case ConflictType::ExactMatch:
        existingConfigurations[duplicateName] = currentConfiguration;
        break;
    case ConflictType::Intersection:
        return false;
    }

    saveConfigurations(filename, existingConfigurations);
    return true;
}

ConflictType YamlHandler::findDuplicateConfiguration(
    const std::map<std::string, Configuration> &configurations,
    const Configuration &currentConfiguration,
    std::string &duplicateName)
{
    bool nameMatches = false;
    bool markerIdMatches = false;
    std::string nameMatchConfig;
    std::string markerIdMatchConfig;

    // Проверка на совпадения и пересечения
    for (const auto &entry : configurations) {
        if (entry.second.name == currentConfiguration.name) {
            nameMatches = true;
            nameMatchConfig = entry.first;
        }
        if (entry.second.markerIds == currentConfiguration.markerIds) {
            markerIdMatches = true;
            markerIdMatchConfig = entry.first;
        }

        std::unordered_set<int>
            existingMarkerIds(entry.second.markerIds.begin(), entry.second.markerIds.end());
        std::unordered_set<int> currentMarkerIds(
            currentConfiguration.markerIds.begin(), currentConfiguration.markerIds.end());

        std::vector<int> existingSortedMarkerIds(existingMarkerIds.begin(), existingMarkerIds.end());
        std::vector<int> currentSortedMarkerIds(currentMarkerIds.begin(), currentMarkerIds.end());
        std::sort(existingSortedMarkerIds.begin(), existingSortedMarkerIds.end());
        std::sort(currentSortedMarkerIds.begin(), currentSortedMarkerIds.end());

        std::vector<int> intersection;
        std::set_intersection(
            existingSortedMarkerIds.begin(),
            existingSortedMarkerIds.end(),
            currentSortedMarkerIds.begin(),
            currentSortedMarkerIds.end(),
            std::back_inserter(intersection));

        if (!intersection.empty() && intersection.size() != existingSortedMarkerIds.size()) {
            duplicateName = "";
            return ConflictType::Intersection;
        }
    }

    if (nameMatches && !markerIdMatches) {
        duplicateName = nameMatchConfig;
        return ConflictType::ExactMatch;
    } else if (!nameMatches && markerIdMatches) {
        duplicateName = markerIdMatchConfig;
        return ConflictType::ExactMatch;
    } else if (nameMatches && markerIdMatches && nameMatchConfig == markerIdMatchConfig) {
        duplicateName = nameMatchConfig;
        return ConflictType::ExactMatch;
    } else if (nameMatches && markerIdMatches && nameMatchConfig != markerIdMatchConfig) {
        return ConflictType::Intersection;
    }

    return ConflictType::None;
}
