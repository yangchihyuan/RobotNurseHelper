#ifndef LIBMP_IMPL_H
#define LIBMP_IMPL_H

#include <map>
#include <unordered_map>
#include "libmp.h"
#include "absl/status/status.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gpu_buffer.h"
#include "mediapipe/gpu/gpu_shared_data_internal.h"
#include "mediapipe/util/resource_util.h"


namespace mediapipe {

	class LibMPImpl : public LibMP {
	public:
		LibMPImpl(){}
		~LibMPImpl();

		absl::Status Init(const char* graph, const char* inputStream);
		bool AddOutputStream(const char* outputStream);
		void SetOutputStreamMaxQueueSize(const char* outputStream, int queue_size);
		bool Start();

		bool Process(uint8_t* data, int width, int height, int image_format);
		bool Process_GPU(uint8_t* data, int width, int height, int image_format);
		bool WriteOutputImage_GPU(uint8_t* dst, const void* outputPacketVoid);

		bool WaitUntilIdle();
		int GetOutputQueueSize(const char* outputStream);
		const void* GetOutputPacket(const char* outputStream);
	private:
		mediapipe::CalculatorGraph m_graph;
		std::string m_input_stream;
		std::unordered_map<std::string, absl::StatusOr<OutputStreamPoller>> m_pollers;
		size_t m_frame_timestamp = 0;
		mediapipe::GlCalculatorHelper gpu_helper;
	};
}
#endif // LIBMP_IMPL_H
