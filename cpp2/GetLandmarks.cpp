//2025/3/29 This is LibMP version of GetLandmarks.cpp
#include "GetLandmarks.hpp"

// returns landmark XYZ data for all detected faces (or empty vector if no detections)
// dimensions = (# faces) x (# landmarks/face) x 3
// i.e., each landmark is a 3-float array (X,Y,Z), so the middle vector contains 468 or 478 of these
// and the outermost vector is for each detected face in the frame
std::vector<std::vector<std::array<float, 3>>> get_landmarks(const std::shared_ptr<mediapipe::LibMP>& libmp) {
	std::vector<std::vector<std::array<float, 3>>> normalized_landmarks;

	// I use a unique_ptr for convenience, so that DeletePacket is called automatically
	// You could also manage deletion yourself, manually:
	// const void* packet = face_mesh->GetOutputPacket("multi_face_landmarks");
	// mediapipe::LibMP::DeletePacket(packet);
	std::unique_ptr<const void, decltype(&mediapipe::LibMP::DeletePacket)> lm_packet_ptr(nullptr, mediapipe::LibMP::DeletePacket);

	// Keep getting packets from queue until empty
	while (libmp->GetOutputQueueSize("multi_face_landmarks") > 0) {
		lm_packet_ptr.reset(libmp->GetOutputPacket("multi_face_landmarks"));
	}
	if (lm_packet_ptr.get() == nullptr || mediapipe::LibMP::PacketIsEmpty(lm_packet_ptr.get())) {
		return normalized_landmarks; // return empty vector if no output packets or packet is invalid
	}

	// Create multi_face_landmarks from packet's protobuf data
	size_t num_faces = mediapipe::LibMP::GetPacketProtoMsgVecSize(lm_packet_ptr.get());
	for (int face_num = 0; face_num < num_faces; face_num++) {
		// Get reference to protobuf message for face
		const void* lm_list_proto = mediapipe::LibMP::GetPacketProtoMsgAt(lm_packet_ptr.get(), face_num);
		// Get byte size of protobuf message
		size_t lm_list_proto_size = mediapipe::LibMP::GetProtoMsgByteSize(lm_list_proto);

		// Create buffer to hold protobuf message data; copy data to buffer
		std::shared_ptr<uint8_t[]> proto_data(new uint8_t[lm_list_proto_size]);
		mediapipe::LibMP::WriteProtoMsgData(proto_data.get(), lm_list_proto, static_cast<int>(lm_list_proto_size));

		// Initialize a mediapipe::NormalizedLandmarkList object from the buffer
		mediapipe::NormalizedLandmarkList face_landmarks;
		face_landmarks.ParseFromArray(proto_data.get(), static_cast<int>(lm_list_proto_size));

		// Copy the landmark data to our custom data structure
		normalized_landmarks.emplace_back();
		for (const mediapipe::NormalizedLandmark& lm : face_landmarks.landmark()) {
			normalized_landmarks[face_num].push_back({ lm.x(), lm.y(), lm.z() });
		}
	}

	return normalized_landmarks;
}

//The pose tracking module always returns a single pose, so we don't need to loop over multiple poses.
std::vector<std::vector<std::array<float, 3>>> get_landmarks_pose(const std::shared_ptr<mediapipe::LibMP>& libmp) {
	std::vector<std::vector<std::array<float, 3>>> normalized_landmarks;

	std::unique_ptr<const void, decltype(&mediapipe::LibMP::DeletePacket)> lm_packet_ptr(nullptr, mediapipe::LibMP::DeletePacket);

	// Keep getting packets from queue until empty
	while (libmp->GetOutputQueueSize("pose_landmarks") > 0) {
		lm_packet_ptr.reset(libmp->GetOutputPacket("pose_landmarks"));
	}
	if (lm_packet_ptr.get() == nullptr || mediapipe::LibMP::PacketIsEmpty(lm_packet_ptr.get())) {
		return normalized_landmarks; // return empty vector if no output packets or packet is invalid
	}

	int	pose_num = 0; // pose_num is always 0 for pose tracking
	const void* lm_list_proto = mediapipe::LibMP::GetPacketProtoMsg(lm_packet_ptr.get());
	// Get byte size of protobuf message
	size_t lm_list_proto_size = mediapipe::LibMP::GetProtoMsgByteSize(lm_list_proto);

	// Create buffer to hold protobuf message data; copy data to buffer
	std::shared_ptr<uint8_t[]> proto_data(new uint8_t[lm_list_proto_size]);
	mediapipe::LibMP::WriteProtoMsgData(proto_data.get(), lm_list_proto, static_cast<int>(lm_list_proto_size));

	// Initialize a mediapipe::NormalizedLandmarkList object from the buffer
	mediapipe::NormalizedLandmarkList pose_landmarks;
	pose_landmarks.ParseFromArray(proto_data.get(), static_cast<int>(lm_list_proto_size));

	// Copy the landmark data to our custom data structure
	normalized_landmarks.emplace_back();
	for (const mediapipe::NormalizedLandmark& lm : pose_landmarks.landmark()) {
		normalized_landmarks[pose_num].push_back({ lm.x(), lm.y(), lm.z() });
	}

	return normalized_landmarks;
}

std::vector<std::vector<std::array<float, 3>>> get_landmarks_holistic(const std::shared_ptr<mediapipe::LibMP>& libmp) {
	std::vector<std::vector<std::array<float, 3>>> normalized_landmarks;

	std::unique_ptr<const void, decltype(&mediapipe::LibMP::DeletePacket)> lm_packet_ptr(nullptr, mediapipe::LibMP::DeletePacket);
	std::unique_ptr<const void, decltype(&mediapipe::LibMP::DeletePacket)> lm_packet_ptr2(nullptr, mediapipe::LibMP::DeletePacket);
	std::unique_ptr<const void, decltype(&mediapipe::LibMP::DeletePacket)> lm_packet_ptr3(nullptr, mediapipe::LibMP::DeletePacket);
	std::unique_ptr<const void, decltype(&mediapipe::LibMP::DeletePacket)> lm_packet_ptr4(nullptr, mediapipe::LibMP::DeletePacket);

	// Keep getting packets from queue until empty
	while (libmp->GetOutputQueueSize("pose_landmarks") > 0) {
		lm_packet_ptr.reset(libmp->GetOutputPacket("pose_landmarks"));
	}
	while (libmp->GetOutputQueueSize("left_hand_landmarks") > 0) {
		lm_packet_ptr2.reset(libmp->GetOutputPacket("left_hand_landmarks"));
	}
	while (libmp->GetOutputQueueSize("right_hand_landmarks") > 0) {
		lm_packet_ptr3.reset(libmp->GetOutputPacket("right_hand_landmarks"));
	}
	while (libmp->GetOutputQueueSize("face_landmarks") > 0) {
		lm_packet_ptr4.reset(libmp->GetOutputPacket("face_landmarks"));
	}
	
	if (lm_packet_ptr.get() == nullptr || mediapipe::LibMP::PacketIsEmpty(lm_packet_ptr.get())) {
		return normalized_landmarks; // return empty vector if no output packets or packet is invalid
	}

	int	pose_num = 0; // pose_num is always 0 for pose tracking
	const void* lm_list_proto = mediapipe::LibMP::GetPacketProtoMsg(lm_packet_ptr.get());
	// Get byte size of protobuf message
	size_t lm_list_proto_size = mediapipe::LibMP::GetProtoMsgByteSize(lm_list_proto);

	// Create buffer to hold protobuf message data; copy data to buffer
	std::shared_ptr<uint8_t[]> proto_data(new uint8_t[lm_list_proto_size]);
	mediapipe::LibMP::WriteProtoMsgData(proto_data.get(), lm_list_proto, static_cast<int>(lm_list_proto_size));

	// Initialize a mediapipe::NormalizedLandmarkList object from the buffer
	mediapipe::NormalizedLandmarkList pose_landmarks;
	pose_landmarks.ParseFromArray(proto_data.get(), static_cast<int>(lm_list_proto_size));

	// Copy the landmark data to our custom data structure
	normalized_landmarks.emplace_back();
	for (const mediapipe::NormalizedLandmark& lm : pose_landmarks.landmark()) {
		normalized_landmarks[pose_num].push_back({ lm.x(), lm.y(), lm.z() });
	}

	return normalized_landmarks;
}
