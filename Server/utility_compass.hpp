#ifndef UTILITY_COMPASS_HPP
#define UTILITY_COMPASS_HPP

#include <opencv2/opencv.hpp>
#include <string>

/**
 * Computes the visual compass theta (robot orientation angle in degrees).
 *
 * @param queryImg The query image to match.
 * @param imageSaveDirectory The base directory where "VisualCompass" photos are saved.
 * @param errorDetails Detailed error/status information populated on failure or matching.
 * @return The estimated orientation angle in degrees [0, 360), or -1.0 if estimation fails.
 */
double ComputeVisualCompassTheta(const cv::Mat &queryImg, 
                                 const std::string &imageSaveDirectory, 
                                 std::string &errorDetails);

#endif // UTILITY_COMPASS_HPP
