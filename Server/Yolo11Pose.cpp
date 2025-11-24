//code from https://github.com/NyiNyiMyo/Cpp-Pose-Estimation-using-YOLOv11-v8-in-onnx/blob/main/pose_estimation_onnx.cpp

#include "Yolo11Pose.hpp"

//#include <onnxruntime_cxx_api.h>
//#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <iomanip>

// -------------------------
// Simple IoU for boxes in xyxy format
// -------------------------
static float bbox_iou_xyxy(const cv::Rect2f& a, const cv::Rect2f& b) {
    float xx1 = std::max(a.x, b.x);
    float yy1 = std::max(a.y, b.y);
    float xx2 = std::min(a.x + a.width, b.x + b.width);
    float yy2 = std::min(a.y + a.height, b.y + b.height);
    float w = std::max(0.0f, xx2 - xx1);
    float h = std::max(0.0f, yy2 - yy1);
    float inter = w * h;
    float areaA = a.width * a.height;
    float areaB = b.width * b.height;
    return inter / (areaA + areaB - inter + 1e-6f);
}

// -------------------------
// NMS
// -------------------------
std::vector<int> nms_indices(const std::vector<cv::Rect2f>& boxes,
    const std::vector<float>& scores,
    float iou_thresh) {
    std::vector<int> idxs(boxes.size());
    std::iota(idxs.begin(), idxs.end(), 0);
    std::sort(idxs.begin(), idxs.end(), [&](int a, int b) { return scores[a] > scores[b]; });

    std::vector<int> keep;
    while (!idxs.empty()) {
        int cur = idxs[0];
        keep.push_back(cur);
        std::vector<int> rem;
        for (size_t i = 1; i < idxs.size(); ++i) {
            int j = idxs[i];
            if (bbox_iou_xyxy(boxes[cur], boxes[j]) <= iou_thresh) rem.push_back(j);
        }
        idxs.swap(rem);
    }
    return keep;
}

// -------------------------
// MAIN
// -------------------------
//int main() {


Yolo11Pose::Yolo11Pose()
:env(ORT_LOGGING_LEVEL_ERROR, "YOLOv11Pose"),
session_options(),
session(env, MODEL_PATH.c_str(), session_options),
input_name_ptr(session.GetInputNameAllocated(0, allocator))
{
    //Set colors
    COLOR_HEAD = cv::Scalar(0, 255, 255);
    COLOR_ARMS = cv::Scalar(200, 120, 0);
    COLOR_BODY = cv::Scalar(0, 255, 0);
    COLOR_LEGS = cv::Scalar(0, 0, 255);


    //Initalize ONNX Runtime
    env = Ort::Env(ORT_LOGGING_LEVEL_ERROR, "YOLOv11Pose");
//    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);

    bool use_cuda = false;
    try {
        OrtCUDAProviderOptions cuda_options;
        session_options.AppendExecutionProvider_CUDA(cuda_options);
        use_cuda = true;
        std::cout << "[INFO] Using CUDA provider\n";
    }
    catch (const std::exception& e) {
        std::cout << "[WARN] CUDA not used: " << e.what() << "\n";
    }

//    Ort::Session session(env, MODEL_PATH.c_str(), session_options);
//    psession = new Ort::Session(env, MODEL_PATH.c_str(), session_options);
//    Ort::AllocatorWithDefaultOptions allocator;

//    Ort::AllocatedStringPtr input_name_ptr = session.GetInputNameAllocated(0, allocator);
    input_name_ptr = session.GetInputNameAllocated(0, allocator);
//    const char* input_name = input_name_ptr.get();
    input_name = input_name_ptr.get();

    size_t out_count = session.GetOutputCount();
//    std::vector<const char*> output_names;
//    std::vector<Ort::AllocatedStringPtr> out_name_ptrs;
    for (size_t i = 0; i < out_count; ++i) {
        out_name_ptrs.push_back(session.GetOutputNameAllocated(i, allocator));
        output_names.push_back(out_name_ptrs.back().get());
    }
}

std::vector<std::vector<std::array<float, 3>>> Yolo11Pose::Process(cv::Mat& frame) {
    std::vector<std::vector<std::array<float, 3>>> return_keypoints;
    try {
            int origW = frame.cols, origH = frame.rows;

            // Preprocess
            cv::Mat img_rgb;
            cv::cvtColor(frame, img_rgb, cv::COLOR_BGR2RGB);
            cv::Mat resized;
            cv::resize(img_rgb, resized, cv::Size(INPUT_SIZE, INPUT_SIZE));
            resized.convertTo(resized, CV_32F, 1.0f / 255.0f);

            std::vector<float> input_tensor_values(1 * 3 * INPUT_SIZE * INPUT_SIZE);
            size_t idx = 0;
            for (int c = 0; c < 3; ++c) {
                for (int y = 0; y < INPUT_SIZE; ++y) {
                    for (int x = 0; x < INPUT_SIZE; ++x) {
                        cv::Vec3f px = resized.at<cv::Vec3f>(y, x);
                        input_tensor_values[idx++] = px[c];
                    }
                }
            }

            std::array<int64_t, 4> input_dims = { 1, 3, INPUT_SIZE, INPUT_SIZE };
            Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
                mem_info, input_tensor_values.data(), input_tensor_values.size(),
                input_dims.data(), input_dims.size()
            );

            auto output_tensors = session.Run(Ort::RunOptions{ nullptr },
                &input_name, &input_tensor, 1,
                output_names.data(), (int)output_names.size());

            if (!output_tensors.empty()) {
//                cv::imshow("Pose", frame);
//                if (cv::waitKey(1) == 27) break;
//                continue;
//                return -1;
//            }

                // Parse output
                Ort::Value& ot = output_tensors[0];
                auto info = ot.GetTensorTypeAndShapeInfo();
                std::vector<int64_t> shape = info.GetShape();
                float* out_ptr = ot.GetTensorMutableData<float>();

                int64_t C = shape[1], N = shape[2];
                int num_kps = (int)((C - 5) / 3);
                std::vector<std::vector<float>> pred(N, std::vector<float>(C));
                for (int64_t c = 0; c < C; ++c)
                    for (int64_t n = 0; n < N; ++n)
                        pred[n][c] = out_ptr[c * N + n];

                std::vector<cv::Rect2f> boxes;
                std::vector<float> scores;
                std::vector<std::vector<float>> kpts;           //keypoints

                for (int64_t i = 0; i < N; ++i) {
                    float obj_score = pred[i][4];
                    if (obj_score < CONF_THRES) continue;

                    float cx = pred[i][0], cy = pred[i][1];
                    float w = pred[i][2], h = pred[i][3];
                    float x1 = cx - w / 2.0f;
                    float y1 = cy - h / 2.0f;

                    boxes.emplace_back(cv::Rect2f(x1, y1, w, h));
                    scores.push_back(obj_score);

                    std::vector<float> kp(num_kps * 3);
                    for (int k = 0; k < num_kps; ++k) {
                        int base = 5 + k * 3;
                        kp[k * 3 + 0] = pred[i][base + 0];
                        kp[k * 3 + 1] = pred[i][base + 1];
                        kp[k * 3 + 2] = pred[i][base + 2];
                    }
                    kpts.push_back(kp);
                }

                if (!boxes.empty()) {
        //                cv::imshow("YOLO Pose Webcam", frame);
        //                if (cv::waitKey(1) == 27) break;
        //                continue;
        //                return -1;
        //            }

                    std::vector<int> keep = nms_indices(boxes, scores, NMS_IOU);

//                    float scale_x = (float)origW / (float)INPUT_SIZE;
//                    float scale_y = (float)origH / (float)INPUT_SIZE;
        //            cv::Mat vis = frame.clone();

                    for (int idx_keep : keep) {
                        // I don't want to draw bounding boxes
                        /*
                        auto& b = boxes[idx_keep];
                        int x1 = std::max(0, (int)std::round(b.x * scale_x));
                        int y1 = std::max(0, (int)std::round(b.y * scale_y));
                        int x2 = std::min(origW - 1, (int)std::round((b.x + b.width) * scale_x));
                        int y2 = std::min(origH - 1, (int)std::round((b.y + b.height) * scale_y));
                        cv::rectangle(vis, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0), 2);
                        */

                        const auto& kp = kpts[idx_keep];
        //                std::vector<cv::Point> scaled_pts(num_kps);
                        std::vector<float> kp_conf(num_kps);

                        std::vector<std::array<float, 3>> keypoints_per_person;
                        for (int k = 0; k < num_kps; ++k) {
//                            float kx = kp[k * 3 + 0] * scale_x;
//                            float ky = kp[k * 3 + 1] * scale_y;
                            float kx = kp[k * 3 + 0] / (float)INPUT_SIZE;
                            float ky = kp[k * 3 + 1] / (float)INPUT_SIZE;
                            float kc = kp[k * 3 + 2];
        //                    scaled_pts[k] = cv::Point((int)kx, (int)ky);
        //                    kp_conf[k] = kc;
        //                    if (kc > KP_CONF_THRES)
        //                        cv::circle(vis, scaled_pts[k], 5, COLOR_HEAD, -1);

                            std::array<float, 3> keypoint = { kx, ky, kc };
                            keypoints_per_person.push_back(keypoint);
                        }
                        return_keypoints.push_back(keypoints_per_person);

                        //Here is the code to draw skeleton
                        //skeleton is defined in the hpp file.
                        /*
                        for (auto& pr : skeleton) {
                            int a = pr.first, b2 = pr.second;
                            if (a < num_kps && b2 < num_kps &&
                                kp_conf[a] > KP_CONF_THRES && kp_conf[b2] > KP_CONF_THRES) {
                                cv::Scalar col = pair_color(a, b2);
                                cv::line(vis, scaled_pts[a], scaled_pts[b2], col, 3, cv::LINE_AA);
                            }
                        }
                        */
                    }
                }
            }

//            cv::imshow("YOLO Pose Webcam", vis);
//            int key = cv::waitKey(1);
//            if (key == 27) break; // ESC to quit
//        }

//        cap.release();
//        cv::destroyAllWindows();
    }
    catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime exception: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
//    return 0;
    return return_keypoints;
}
