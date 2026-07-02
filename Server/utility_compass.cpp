#include "utility_compass.hpp"
#include "utility_directory.hpp"
#include <QDir>
#include <QStringList>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>

double ComputeVisualCompassTheta(const cv::Mat &queryImg, 
                                 const std::string &imageSaveDirectory, 
                                 std::string &errorDetails)
{
    errorDetails.clear();

    // 1. Convert query image to grayscale if it is color
    cv::Mat queryGray;
    if (queryImg.channels() == 3) {
        cv::cvtColor(queryImg, queryGray, cv::COLOR_BGR2GRAY);
    } else {
        queryGray = queryImg;
    }

    // 2. Locate the saved photos in VisualCompass directory
    QString dirPath = QString::fromStdString(ReplaceShellVariable(imageSaveDirectory)) + "/VisualCompass";
    QDir dir(dirPath);
    if (!dir.exists()) {
        std::string err = "Error: VisualCompass directory does not exist at: " + dirPath.toStdString();
        std::cout << err << std::endl;
        errorDetails = err;
        return -1.0;
    }

    QStringList filters;
    filters << "photo_*_*.jpg";
    QStringList fileList = dir.entryList(filters, QDir::Files, QDir::Time);

    std::map<int, QString> referencePhotos;
    std::vector<int> angles;
    for (const QString &fileName : fileList) {
        QStringList parts = fileName.split('_');
        if (parts.size() >= 3 && parts[0] == "photo") {
            bool ok = false;
            int angle = parts[1].toInt(&ok);
            if (ok) {
                if (referencePhotos.find(angle) == referencePhotos.end()) {
                    referencePhotos[angle] = dir.absoluteFilePath(fileName);
                    angles.push_back(angle);
                }
            }
        }
    }

    if (referencePhotos.empty()) {
        std::string err = "Error: No saved photos found in: " + dirPath.toStdString();
        std::cout << err << std::endl;
        errorDetails = err;
        return -1.0;
    }

    // 3. Initialize ORB detector & BFMatcher
    // cv::Ptr<> is OpenCV's smart pointer
    cv::Ptr<cv::ORB> orb = cv::ORB::create(1000); // 1000 features
    std::vector<cv::KeyPoint> queryKeypoints;
    cv::Mat queryDescriptors;
    orb->detectAndCompute(queryGray, cv::noArray(), queryKeypoints, queryDescriptors);

    if (queryDescriptors.empty()) {
        std::string err = "Error: No ORB features detected in the current frame.";
        std::cout << err << std::endl;
        errorDetails = err;
        return -1.0;
    }

    cv::BFMatcher matcher(cv::NORM_HAMMING, true); // crossCheck = true

    std::vector<std::pair<int, int>> angleScores;
    QString scoreDetails = "ORB Matching Scores (Homography Inliers):\n";

    // 4. Compare current frame with each of the saved photos
    for (int angle : angles) {
        if (referencePhotos.find(angle) == referencePhotos.end()) {
            scoreDetails += QString::number(angle) + "°: No photo saved\n";
            continue;
        }

        QString photoPath = referencePhotos[angle];
        cv::Mat dbImg = cv::imread(photoPath.toStdString(), cv::IMREAD_GRAYSCALE);
        if (dbImg.empty()) {
            scoreDetails += QString::number(angle) + "°: Failed to load photo\n";
            continue;
        }

        std::vector<cv::KeyPoint> dbKeypoints;
        cv::Mat dbDescriptors;
        orb->detectAndCompute(dbImg, cv::noArray(), dbKeypoints, dbDescriptors);

        if (dbDescriptors.empty()) {
            scoreDetails += QString::number(angle) + "°: No database ORB features\n";
            continue;
        }

        std::vector<cv::DMatch> matches;
        matcher.match(queryDescriptors, dbDescriptors, matches);

        // Filter good matches
        double minDist = 100.0;
        for (const auto &match : matches) {
            if (match.distance < minDist) minDist = match.distance;
        }
        std::vector<cv::DMatch> goodMatches;
        for (const auto &match : matches) {
            if (match.distance <= std::max(2.0 * minDist, 30.0)) {
                goodMatches.push_back(match);
            }
        }

        // RANSAC homography check for geometric validation
        int score = 0;
        if (goodMatches.size() >= 4) {
            std::vector<cv::Point2f> srcPoints, dstPoints;
            for (const auto &m : goodMatches) {
                srcPoints.push_back(queryKeypoints[m.queryIdx].pt);
                dstPoints.push_back(dbKeypoints[m.trainIdx].pt);
            }
            cv::Mat mask;
            cv::Mat H = cv::findHomography(srcPoints, dstPoints, cv::RANSAC, 3.0, mask);
            if (!H.empty()) {
                score = cv::countNonZero(mask);
            } else {
                score = goodMatches.size();
            }
        } else {
            score = goodMatches.size();
        }

        scoreDetails += QString::number(angle) + "°: " + QString::number(score) + " inliers (from " + photoPath.split('/').last() + ")\n";
        angleScores.push_back({score, angle});
    }

    // Filter valid scores (> 0)
    std::vector<std::pair<int, int>> validScores;
    for (const auto &p : angleScores) {
        if (p.first > 0) {
            validScores.push_back(p);
        }
    }

    if (validScores.empty()) {
        std::string err = "Failed to determine orientation (no matching features).\n\n" + scoreDetails.toStdString();
        std::cout << "Failed to determine orientation (no matching features)." << std::endl;
        errorDetails = err;
        return -1.0;
    }

    // Sort in descending order of scores
    std::sort(validScores.begin(), validScores.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
        return a.first > b.first;
    });

    double estimatedAngle = 0.0;

    if (validScores.size() == 1) {
        estimatedAngle = validScores[0].second;
    } else {
        // Interpolate using the two best matches
        int S_1 = validScores[0].first;         // score of the best match
        int theta1 = validScores[0].second;     // angle of the best match
        int S_2 = validScores[1].first;         // score of the second best match
        int theta2 = validScores[1].second;     // angle of the second best match

        const double PI = 3.14159265358979323846;
        double theta1_rad = theta1 * PI / 180.0;
        double theta2_rad = theta2 * PI / 180.0;

        double x = S_1 * cos(theta1_rad) + S_2 * cos(theta2_rad);
        double y = S_1 * sin(theta1_rad) + S_2 * sin(theta2_rad);
        double interp_rad = atan2(y, x);
        estimatedAngle = interp_rad * 180.0 / PI;
        if (estimatedAngle < 0) {
            estimatedAngle += 360.0;
        }
    }

    return estimatedAngle;
}
