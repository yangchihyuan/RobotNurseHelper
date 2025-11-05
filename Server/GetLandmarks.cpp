//2025/3/29 This is LibMP version of GetLandmarks.cpp
#include "GetLandmarks.hpp"

// returns landmark XYZ data for all detected faces (or empty vector if no detections)
// dimensions = (# faces) x (# landmarks/face) x 3
// i.e., each landmark is a 3-float array (X,Y,Z), so the middle vector contains 468 or 478 of these
// and the outermost vector is for each detected face in the frame
std::vector<std::vector<std::array<float, 3>>> get_landmarks_face(const std::shared_ptr<mediapipe::LibMP>& libmp) {
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

std::vector<std::vector<std::array<float, 3>>> get_landmarks_hand(const std::shared_ptr<mediapipe::LibMP>& libmp) {
	std::vector<std::vector<std::array<float, 3>>> normalized_landmarks;

	// I use a unique_ptr for convenience, so that DeletePacket is called automatically
	// You could also manage deletion yourself, manually:
	// const void* packet = face_mesh->GetOutputPacket("multi_face_landmarks");
	// mediapipe::LibMP::DeletePacket(packet);
	std::unique_ptr<const void, decltype(&mediapipe::LibMP::DeletePacket)> lm_packet_ptr(nullptr, mediapipe::LibMP::DeletePacket);

	// Keep getting packets from queue until empty
	while (libmp->GetOutputQueueSize("landmarks") > 0) {
		lm_packet_ptr.reset(libmp->GetOutputPacket("landmarks"));
	}
	if (lm_packet_ptr.get() == nullptr || mediapipe::LibMP::PacketIsEmpty(lm_packet_ptr.get())) {
		return normalized_landmarks; // return empty vector if no output packets or packet is invalid
	}

	// Create multi_face_landmarks from packet's protobuf data
	size_t num_hands = mediapipe::LibMP::GetPacketProtoMsgVecSize(lm_packet_ptr.get());
	for (int hand_num = 0; hand_num < num_hands; hand_num++) {
		// Get reference to protobuf message for face
		const void* lm_list_proto = mediapipe::LibMP::GetPacketProtoMsgAt(lm_packet_ptr.get(), hand_num);
		// Get byte size of protobuf message
		size_t lm_list_proto_size = mediapipe::LibMP::GetProtoMsgByteSize(lm_list_proto);

		// Create buffer to hold protobuf message data; copy data to buffer
		std::shared_ptr<uint8_t[]> proto_data(new uint8_t[lm_list_proto_size]);
		mediapipe::LibMP::WriteProtoMsgData(proto_data.get(), lm_list_proto, static_cast<int>(lm_list_proto_size));

		// Initialize a mediapipe::NormalizedLandmarkList object from the buffer
		mediapipe::NormalizedLandmarkList hand_landmarks;
		hand_landmarks.ParseFromArray(proto_data.get(), static_cast<int>(lm_list_proto_size));

		// Copy the landmark data to our custom data structure
		normalized_landmarks.emplace_back();
		for (const mediapipe::NormalizedLandmark& lm : hand_landmarks.landmark()) {
			normalized_landmarks[hand_num].push_back({ lm.x(), lm.y(), lm.z() });
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

	int	pose_num = 0; // pose_num is always 0 for holistic tracking
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

// Now I need the hand landmarks. What format should I return them in?
HolisticLandmarks get_landmarks_holistic2(const std::shared_ptr<mediapipe::LibMP>& libmp) {
	HolisticLandmarks holistic_landmarks;

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
		return holistic_landmarks;
	}

	// Get pose landmarks
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
	holistic_landmarks.pose.emplace_back();
	for (const mediapipe::NormalizedLandmark& lm : pose_landmarks.landmark()) {
		holistic_landmarks.pose.push_back({ lm.x(), lm.y(), lm.z() });
	}

	// Get left hand landmarks
	// There maybe no left hand landmarks detected, so we check for that
	if (lm_packet_ptr2.get() == nullptr || mediapipe::LibMP::PacketIsEmpty(lm_packet_ptr2.get())) {
		holistic_landmarks.left_hand.clear();
	} else {
		const void* lm_list_proto2 = mediapipe::LibMP::GetPacketProtoMsg(lm_packet_ptr2.get());
		// Get byte size of protobuf message
		size_t lm_list_proto_size2 = mediapipe::LibMP::GetProtoMsgByteSize(lm_list_proto2);

		// Create buffer to hold protobuf message data; copy data to buffer
		std::shared_ptr<uint8_t[]> proto_data2(new uint8_t[lm_list_proto_size2]);
		mediapipe::LibMP::WriteProtoMsgData(proto_data2.get(), lm_list_proto2, static_cast<int>(lm_list_proto_size2));

		// Initialize a mediapipe::NormalizedLandmarkList object from the buffer
		mediapipe::NormalizedLandmarkList left_hand_landmarks;
		left_hand_landmarks.ParseFromArray(proto_data2.get(), static_cast<int>(lm_list_proto_size2));

		// Copy the landmark data to our custom data structure
		holistic_landmarks.left_hand.emplace_back();
		for (const mediapipe::NormalizedLandmark& lm : left_hand_landmarks.landmark()) {
			holistic_landmarks.left_hand.push_back({ lm.x(), lm.y(), lm.z() });
		}
	}

	// Get right hand landmarks
	// There maybe no right hand landmarks detected, so we check for that
	if (lm_packet_ptr3.get() == nullptr || mediapipe::LibMP::PacketIsEmpty(lm_packet_ptr3.get())) {
		holistic_landmarks.right_hand.clear();
	} else {
		// Get right hand landmarks
		const void* lm_list_proto3 = mediapipe::LibMP::GetPacketProtoMsg(lm_packet_ptr3.get());
		// Get byte size of protobuf message
		size_t lm_list_proto_size3 = mediapipe::LibMP::GetProtoMsgByteSize(lm_list_proto3);
		// Create buffer to hold protobuf message data; copy data to buffer
		std::shared_ptr<uint8_t[]> proto_data3(new uint8_t[lm_list_proto_size3]);
		mediapipe::LibMP::WriteProtoMsgData(proto_data3.get(), lm_list_proto3, static_cast<int>(lm_list_proto_size3));

		// Initialize a mediapipe::NormalizedLandmarkList object from the buffer
		mediapipe::NormalizedLandmarkList right_hand_landmarks;
		right_hand_landmarks.ParseFromArray(proto_data3.get(), static_cast<int>(lm_list_proto_size3));

		// Copy the landmark data to our custom data structure
		holistic_landmarks.right_hand.emplace_back();
		for (const mediapipe::NormalizedLandmark& lm : right_hand_landmarks.landmark()) {
			holistic_landmarks.right_hand.push_back({ lm.x(), lm.y(), lm.z() });
		}
	}

	// Get face landmarks
	if (lm_packet_ptr4.get() == nullptr || mediapipe::LibMP::PacketIsEmpty(lm_packet_ptr4.get())) {
		holistic_landmarks.face.clear();
	} else {
		// Get face landmarks
		// Get the face landmarks packet
		const void* lm_list_proto4 = mediapipe::LibMP::GetPacketProtoMsg(lm_packet_ptr4.get());
		// Get byte size of protobuf message
		size_t lm_list_proto_size4 = mediapipe::LibMP::GetProtoMsgByteSize(lm_list_proto4);
		// Create buffer to hold protobuf message data; copy data to buffer
		std::shared_ptr<uint8_t[]> proto_data4(new uint8_t[lm_list_proto_size4]);
		mediapipe::LibMP::WriteProtoMsgData(proto_data4.get(), lm_list_proto4, static_cast<int>(lm_list_proto_size4));

		// Initialize a mediapipe::NormalizedLandmarkList object from the buffer
		mediapipe::NormalizedLandmarkList face_landmarks;
		face_landmarks.ParseFromArray(proto_data4.get(), static_cast<int>(lm_list_proto_size4));

		// Copy the landmark data to our custom data structure
		holistic_landmarks.face.emplace_back();
		for (const mediapipe::NormalizedLandmark& lm : face_landmarks.landmark()) {
			holistic_landmarks.face.push_back({ lm.x(), lm.y(), lm.z() });
		}
	}

	return holistic_landmarks;
}
