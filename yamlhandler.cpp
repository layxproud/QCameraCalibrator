#include "yamlhandler.h"
#include <fstream>
#include <sstream>

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

    // First pass to identify potential matches
    for (const auto &entry : configurations) {
        if (entry.second.name == currentConfiguration.name) {
            nameMatches = true;
            nameMatchConfig = entry.first;
        }
        if (entry.second.markerIds == currentConfiguration.markerIds) {
            markerIdMatches = true;
            markerIdMatchConfig = entry.first;
        }
    }

    // Check for intersection
    std::vector<int> intersection;
    bool intersectsExisting = false;
    for (const auto &entry : configurations) {
        std::set_intersection(
            entry.second.markerIds.begin(),
            entry.second.markerIds.end(),
            currentConfiguration.markerIds.begin(),
            currentConfiguration.markerIds.end(),
            std::back_inserter(intersection));
        if (!intersection.empty()) {
            intersectsExisting = true;
            break;
        }
    }

    // Determine conflict type based on matches and intersections
    if (nameMatches && markerIdMatches && nameMatchConfig != markerIdMatchConfig) {
        // Special case: name matches one config, marker IDs match another
        duplicateName = nameMatchConfig;   // Or choose another strategy to report both
        return ConflictType::Intersection; // Treat as intersection to prevent overwrite
    } else if (intersectsExisting) {
        duplicateName = ""; // No specific config to reference, just indicate intersection
        return ConflictType::Intersection;
    } else if (nameMatches || markerIdMatches) {
        duplicateName = nameMatches ? nameMatchConfig : markerIdMatchConfig;
        return ConflictType::ExactMatch;
    }

    return ConflictType::None;
}
