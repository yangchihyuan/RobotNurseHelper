#include <array>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "libmp.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/formats/image_format.pb.h"

std::vector<std::vector<std::array<float, 3>>> get_landmarks_face(const std::shared_ptr<mediapipe::LibMP>& libmp);
std::vector<std::vector<std::array<float, 3>>> get_landmarks_pose(const std::shared_ptr<mediapipe::LibMP>& libmp);
std::vector<std::vector<std::array<float, 3>>> get_landmarks_holistic(const std::shared_ptr<mediapipe::LibMP>& libmp);