#include "libmp_impl_gpu.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "google/protobuf/message_lite.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include <opencv2/opencv.hpp>

namespace mediapipe {

	LibMPImpl::~LibMPImpl()
	{
		LOG(INFO) << "Shutting down.";
		absl::Status status = m_graph.CloseInputStream(m_input_stream);
		if (status.ok()){
			absl::Status status1 = m_graph.WaitUntilDone();
			if (!status1.ok()){
				LOG(INFO) << "Error in WaitUntilDone(): " << status1.ToString();
			}
		} else {
			LOG(INFO) << "Error in CloseInputStream(): " << status.ToString();
		}
	}

	absl::Status LibMPImpl::Init(const char* graph, const char* inputStream){
		mediapipe::CalculatorGraphConfig config = mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(graph);
		MP_RETURN_IF_ERROR(m_graph.Initialize(config));
		m_input_stream.assign(inputStream);
		LOG(INFO) << "Successfully initialized LibMP graph";

		return absl::OkStatus();
	}

	absl::Status LibMPImpl::Init_gpu(const char* graph, const char* inputStream){
		mediapipe::CalculatorGraphConfig config = mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(graph);
		MP_RETURN_IF_ERROR(m_graph.Initialize(config));
		ABSL_LOG(INFO) << "Initialize the GPU.";
		MP_ASSIGN_OR_RETURN(auto gpu_resources, mediapipe::GpuResources::Create());
		MP_RETURN_IF_ERROR(m_graph.SetGpuResources(std::move(gpu_resources)));
		m_input_stream.assign(inputStream);

		gpu_helper.InitializeForTest(m_graph.GetGpuResources().get());

		LOG(INFO) << "Successfully initialized LibMP graph";
		return absl::OkStatus();
	}

	bool LibMPImpl::AddOutputStream(const char* outputStream){
		m_pollers.emplace(outputStream, m_graph.AddOutputStreamPoller(outputStream));
		return m_pollers.at(outputStream).ok() ? true : false;
	}

	void LibMPImpl::SetOutputStreamMaxQueueSize(const char* outputStream, int queue_size){
		m_pollers.at(outputStream)->SetMaxQueueSize(queue_size);
	}

	bool LibMPImpl::Start(){
		const std::map<std::string, mediapipe::Packet>& extra_side_packets = {};
		bool ok = m_graph.StartRun(extra_side_packets).ok();
		LOG(INFO) << (ok ? "Started " : "Failed to start ") << "calculator graph";
		return ok;
	}

	bool LibMPImpl::Process(uint8_t* data, int width, int height, int image_format)
	{
		if (data == nullptr){
			LOG(INFO) << __FUNCTION__ << " input data is nullptr!";
			return false;
		}
		if (!mediapipe::ImageFormat::Format_IsValid(image_format)){
			LOG(INFO) << __FUNCTION__ << " input image format (" << image_format << ") is invalid!";
			return false;
		}

		// copy input data to ImgFrame
		auto input_frame_for_input = std::make_unique<ImageFrame>();
		auto mp_image_format = static_cast<mediapipe::ImageFormat::Format>(image_format);
		input_frame_for_input->CopyPixelData(mp_image_format, width, height, data, mediapipe::ImageFrame::kDefaultAlignmentBoundary);

		//2025/4/2 Debug information: The author of LibMP uses an incorrect timestamp so that pose tracking and holistic tracking become very slow.
		size_t frame_timestamp_us = (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;
		auto status = m_graph.AddPacketToInputStream(m_input_stream, mediapipe::Adopt(input_frame_for_input.release()).At(mediapipe::Timestamp(frame_timestamp_us)));


		if (!status.ok()){
			LOG(INFO) << "Failed to add packet to input stream. Call m_graph.WaitUntilDone() to see error (or destroy LibMP object)";
			LOG(INFO) << "Status: " << status.ToString() << std::endl;
			return false;
		}
		return true;
	}

	bool LibMPImpl::Process2(cv::Mat camera_frame)
	{
		auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
			mediapipe::ImageFormat::SRGB, camera_frame.cols, camera_frame.rows,
			mediapipe::ImageFrame::kDefaultAlignmentBoundary);
		cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
		camera_frame.copyTo(input_frame_mat);
		size_t frame_timestamp_us =
        (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;
		auto status = m_graph.AddPacketToInputStream(m_input_stream, mediapipe::Adopt(input_frame.release()).At(mediapipe::Timestamp(frame_timestamp_us)));
			
		if (!status.ok()){
			LOG(INFO) << "Failed to add packet to input stream. Call m_graph.WaitUntilDone() to see error (or destroy LibMP object)";
			LOG(INFO) << "Status: " << status.ToString() << std::endl;
			return false;
		}
		return true;
	}

	bool LibMPImpl::Process_GPU(uint8_t* data, int width, int height, int image_format)
	{
		if (data == nullptr){
			LOG(INFO) << __FUNCTION__ << " input data is nullptr!";
			return false;
		}
		if (!mediapipe::ImageFormat::Format_IsValid(image_format)){
			LOG(INFO) << __FUNCTION__ << " input image format (" << image_format << ") is invalid!";
			return false;
		}

		// copy input data to ImgFrame
		auto input_frame = std::make_unique<ImageFrame>();
		auto mp_image_format = static_cast<mediapipe::ImageFormat::Format>(image_format);
		input_frame->CopyPixelData(mp_image_format, width, height, data, mediapipe::ImageFrame::kDefaultAlignmentBoundary);

		//2025/4/2 Debug information: The author of LibMP uses an incorrect timestamp so that pose tracking and holistic tracking become very slow.
		size_t frame_timestamp_us = (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;

    	// Prepare and add graph input packet.
			gpu_helper.RunInGlContext([&input_frame, &frame_timestamp_us, this]() -> absl::Status {
			  // Convert ImageFrame to GpuBuffer.
			  auto texture = this->gpu_helper.CreateSourceTexture(*input_frame.get());
			  auto gpu_frame = texture.GetFrame<mediapipe::GpuBuffer>();
			  glFlush();
			  texture.Release();
			  // Send GPU image packet into the graph.
			  MP_RETURN_IF_ERROR(this->m_graph.AddPacketToInputStream(
				this->m_input_stream, mediapipe::Adopt(gpu_frame.release())
									.At(mediapipe::Timestamp(frame_timestamp_us))));
			  return absl::OkStatus();
			});
//			auto status = m_graph.AddPacketToInputStream(m_input_stream, mediapipe::Adopt(input_frame.release()).At(mediapipe::Timestamp(frame_timestamp_us)));

/*
		if (!status.ok()){
			LOG(INFO) << "Failed to add packet to input stream. Call m_graph.WaitUntilDone() to see error (or destroy LibMP object)";
			LOG(INFO) << "Status: " << status.ToString() << std::endl;
			return false;
		}
			*/
		return true;
	}

	bool LibMPImpl::WriteOutputImage_GPU(uint8_t* dst, const void* outputPacketVoid){
		auto packet = reinterpret_cast<const mediapipe::Packet*>(outputPacketVoid);
		std::unique_ptr<mediapipe::ImageFrame> output_frame;

		gpu_helper.RunInGlContext(
			[packet, &output_frame, this]() -> absl::Status {
			  auto& gpu_frame = packet->Get<mediapipe::GpuBuffer>();
			  auto texture = this->gpu_helper.CreateSourceTexture(gpu_frame);
			  output_frame = absl::make_unique<mediapipe::ImageFrame>(
				  mediapipe::ImageFormatForGpuBufferFormat(gpu_frame.format()),
				  gpu_frame.width(), gpu_frame.height(),
				  mediapipe::ImageFrame::kGlDefaultAlignmentBoundary);
			  this->gpu_helper.BindFramebuffer(texture);
			  const auto info = mediapipe::GlTextureInfoForGpuBufferFormat(
				  gpu_frame.format(), 0, this->gpu_helper.GetGlVersion());
			  glReadPixels(0, 0, texture.width(), texture.height(), info.gl_format,
						   info.gl_type, output_frame->MutablePixelData());
			  glFlush();
			  texture.Release();
			  return absl::OkStatus();
			});

		// Convert back to opencv for display or saving.
		cv::Mat output_frame_mat = mediapipe::formats::MatView(output_frame.get());
		if (output_frame_mat.channels() == 4)
//			cv::cvtColor(output_frame_mat, output_frame_mat, cv::COLOR_RGBA2BGR);
			cv::cvtColor(output_frame_mat, output_frame_mat, cv::COLOR_RGBA2RGB);      //This line takes effect
//		else
//			cv::cvtColor(output_frame_mat, output_frame_mat, cv::COLOR_RGB2BGR);


		//The number is too large. Maybe it does not work for the GPU case.
//		size_t output_bytes = output_frame->PixelDataSizeStoredContiguously();
		//This line fails.
//		output_frame->CopyToBuffer(dst, output_bytes);
		memcpy(dst, output_frame_mat.data, output_frame_mat.total() * output_frame_mat.elemSize());

		delete packet;
		return true;
	}

	bool LibMPImpl::WaitUntilIdle(){
		return m_graph.WaitUntilIdle().ok();
	}

	int LibMPImpl::GetOutputQueueSize(const char* outputStream){
		return m_pollers.at(outputStream)->QueueSize();
	}

	const void* LibMPImpl::GetOutputPacket(const char* outputStream){
		if (m_pollers.find(outputStream) == m_pollers.end()){
			LOG(INFO) << "No poller found for output stream '" << outputStream << "'. Was it created using AddOutputStream beforehand?";
			return nullptr;
		}
		auto outputPacket = std::make_unique<mediapipe::Packet>();
		if (!m_pollers.at(outputStream)->Next(outputPacket.get())){
			LOG(INFO) << "Poller for output stream '" << outputStream << "' has no next packet. Call m_graph.WaitUntilDone() to see error (or destroy LibMP object). Are models available under mediapipe/models and mediapipe/modules?";
			return nullptr;
		}
		return reinterpret_cast<const void*>(outputPacket.release());
	}

}
