#include <array>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "libmp.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/formats/image_format.pb.h"

struct HolisticLandmarks {
	std::vector<std::array<float, 3>> pose;
	std::vector<std::array<float, 3>> left_hand;
	std::vector<std::array<float, 3>> right_hand;
	std::vector<std::array<float, 3>> face;
};

std::vector<std::vector<std::array<float, 3>>> get_landmarks_face(const std::shared_ptr<mediapipe::LibMP>& libmp);
std::vector<std::vector<std::array<float, 3>>> get_landmarks_hand(const std::shared_ptr<mediapipe::LibMP>& libmp);
std::vector<std::vector<std::array<float, 3>>> get_landmarks_pose(const std::shared_ptr<mediapipe::LibMP>& libmp);
std::vector<std::vector<std::array<float, 3>>> get_landmarks_holistic(const std::shared_ptr<mediapipe::LibMP>& libmp);
HolisticLandmarks get_landmarks_holistic2(const std::shared_ptr<mediapipe::LibMP>& libmp);
