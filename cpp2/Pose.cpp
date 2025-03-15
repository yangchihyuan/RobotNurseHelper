//#include "human_pose_estimator.hpp"

#include "Pose.hpp"
//#include "include/human_pose_estimator.hpp"
#include <vector>
//#include "cnpy.h" //for save numpy array
#include <cmath>
//#include "Tensor.hpp"
#include <limits>   //Chih-Yuan Yang: What is this?
#include "Logger.hpp"
//#include "TensorMatConversion.hpp"
#include "utility_compute.hpp"

//using namespace human_pose_estimation;
using namespace cv;

/*
int ConvertImageToPoseMaps(const string& input_dir, 
                           const vector<string>& file_list, 
                           const string& output_dir, 
                           const string& pose_model)
{
    HumanPoseEstimator estimator(pose_model, "CPU", false); //the 3rd argument is per-layer performance report

    //load images
    for( int i=0; i< static_cast<int>(file_list.size()); i++ )
    {
        string fullfilename = input_dir + "/" + file_list[i];
        cout << i << " " << fullfilename << endl;
        Mat inputImage = imread(fullfilename);

        //create pose maps
        const int height = inputImage.rows;
        const int width = inputImage.cols;

        //save a numpy array
        string output_filename = output_dir + "/" + file_list[i] + ".npy";
        const int number_layers = 18;
        vector<float> data(width*height*number_layers, 0.0f);

        vector<HumanPose> poses = estimator.estimate(inputImage);
        if( poses.size() > 0)
        {
            HumanPose pose = poses[0];      //Assume there is only 1 person on an image.
            vector<Point2f> keypoints = pose.keypoints;
            const int radius = 12;
            const int radius_square = radius * radius;
            for(int point_id=0; point_id<18; point_id++)
            {
                const Point2f point = keypoints[point_id];            //Point2f is a OpenCV struct
                const int x_int = static_cast<int>(round(point.x));
                const int y_int = static_cast<int>(round(point.y));

                if( x_int != -1 && y_int != -1)
                {
                    //draw circles with radius 12
                    const int x_min = max(0, x_int-radius);
                    const int x_max = min(width-1, x_int + radius);
                    const int y_min = max(0, y_int-radius);
                    const int y_max = min(height-1, y_int+radius);
                    int dx, dy, dist_square;
                    for( int x = x_min; x<=x_max ; x++)
                        for( int y=y_min; y<=y_max; y++)
                        {
                            dx = x - x_int;
                            dy = y - y_int;
                            dist_square = dx * dx + dy * dy;
                            if( dist_square <= radius_square)
                            {
                                data[point_id + number_layers*x + y*width*number_layers] = 1.0f;
                            }
                        }
                }
            }
        }

        const size_t Nx = static_cast<size_t>(width);
        const size_t Ny = static_cast<size_t>(height);
        const size_t Nz = static_cast<size_t>(number_layers);
        cnpy::npy_save(output_filename,&data[0],{Ny,Nx,Nz},"w");
    }
    return 1;
}
*/


/*
int ConvertImageToPoses(const Mat& input_image, const string& pose_model, vector<HumanPose>& poses )
{
    HumanPoseEstimator estimator(pose_model, "CPU", false); //the 3rd argument is per-layer performance report
    poses = estimator.estimate(input_image);
    return 1;
}
*/

/*
int ConvertPoseToTensor(const HumanPose& pose, tensorflow::Tensor &tensor)
{
    vector<int> shape = get_tensor_shape(tensor);
    const int height = shape[2];
    const int width = shape[3];


    auto input_tensor_mapped = tensor.tensor<float, 4>();
    vector<Point2f> keypoints = pose.keypoints;
    const int radius = 12;
    const int radius_square = radius * radius;
    for(int point_id=0; point_id<18; point_id++)
    {
        const Point2f point = keypoints[point_id];            //Point2f is a OpenCV struct
        const int x_int = static_cast<int>(round(point.x));
        const int y_int = static_cast<int>(round(point.y));

        if( x_int != -1 && y_int != -1)
        {
            //draw circles with radius 12
            const int x_min = max(0, x_int-radius);
            const int x_max = min(width-1, x_int + radius);
            const int y_min = max(0, y_int-radius);
            const int y_max = min(height-1, y_int+radius);
            int dx, dy, dist_square;
            for( int x = x_min; x<=x_max ; x++)
                for( int y=y_min; y<=y_max; y++)
                {
                    dx = x - x_int;
                    dy = y - y_int;
                    dist_square = dx * dx + dy * dy;
                    if( dist_square <= radius_square)
                    {
                        input_tensor_mapped(0, point_id, y, x) = 1.0f;
                    }
                }
        }
    }
    return 1;
}
*/

/*
vector<PoseRegion> CropRegionsFromPoses(const Mat& inputImage, const vector<HumanPose>& poses)
{
    vector<PoseRegion> return_vector;
    for(unsigned int i=0; i< poses.size(); i++)
    {
        HumanPose pose = poses[i];
        if( pose.keypoints[1].x != -1.0f && pose.keypoints[1].y != -1.0f)   //ignore some poses without the center point
        {
            Rect region = GetPoseRegion(pose);
            Rect region_expanded = region;
            int expand_width = 12;
            if(region_expanded.x < expand_width)
                region_expanded.x = 0;
            else
                region_expanded.x -= expand_width;
                
            if( region_expanded.y < expand_width)
                region_expanded.y = 0;
            else
                region_expanded.y -= expand_width;
                
            if( region_expanded.x + region_expanded.width + expand_width*2 > inputImage.cols)
                region_expanded.width = inputImage.cols - region_expanded.x;
            else
                region_expanded.width += expand_width*2;

            if( region_expanded.y + region_expanded.height + expand_width*2 > inputImage.rows)
                region_expanded.height = inputImage.rows - region_expanded.y;
            else
                region_expanded.height += expand_width*2;

    //        Logger( "region_expanded " + cv::format("(%dx%d)(%d %d)\n", region_expanded.width, region_expanded.height, region_expanded.x, region_expanded.y));

            HumanPose pose_offset = pose;
            for( unsigned int j=0; j<pose.keypoints.size(); j++)
            {
                pose_offset.keypoints[j].x -= region_expanded.x;
                pose_offset.keypoints[j].y -= region_expanded.y;
            }
            PoseRegion pose_region;
            pose_region.mat = inputImage(region_expanded);
            pose_region.pose = pose_offset;
            pose_region.index_in_poses = i;

            return_vector.push_back(pose_region);
        }
    }
    return return_vector;
}
*/
Rect GetPoseRegion(const HumanPose& pose)
{
    float top, bottom, right, left;
    top = numeric_limits<float>::max();
    bottom = numeric_limits<float>::min();
    right = numeric_limits<float>::min();
    left = numeric_limits<float>::max();
    for(unsigned int i=0; i<pose.keypoints.size(); i++)
    {
        const float x = pose.keypoints[i].x;
        const float y = pose.keypoints[i].y;
        if( x != -1.0f && y != -1.0f)
        {
            if( y < top)
                top = y;
            if( y > bottom)
                bottom = y;
            if( x > right)
                right = x;
            if( x < left)
                left = x;
        }
    }
    //Rect constructor(x,y,width,height)
    Rect region(static_cast<int>(left), static_cast<int>(top), static_cast<int>(right-left), static_cast<int>(bottom-top));
//    Logger( "region " + cv::format("(%dx%d)(%d %d)\n", region.width, region.height, region.x, region.y));
    return region;
}

//Sort OpenVINO's openpose results by the distance between neck to nose, eyes, and ears
vector<int> SortPosesByNeckToNose(const vector<HumanPose>& poses)
{
    //compute the distnaces
    vector<float> distances(poses.size(),0);
    for( unsigned int idx = 0; idx < poses.size(); idx++ )
    {
        HumanPose pose = poses.at(idx);
        auto keypoint_center = pose.keypoints[1];

        if(keypoint_center.x != -1 && keypoint_center.y != -1)
        {
            int facial_component_array[5] = {0, 14, 15, 16, 17};
            float sum = 0;
            int count = 0;
            for(int facial_component_index : facial_component_array)
            {
                auto keypoint_component = pose.keypoints[facial_component_index];
                if(keypoint_component.x != -1 && keypoint_component.y != -1)
                {
                    count += 1;
                    float diff_x = keypoint_center.x - keypoint_component.x;
                    float diff_y = keypoint_center.y - keypoint_component.y;
                    float distance = sqrt(pow(diff_x, 2) + pow(diff_y, 2));
                    sum += distance;
                }
            }

            if( count != 0)
            {
                distances[idx] = sum/count;
            }
        }
    }

    //get sorted indexes, the larger distance, the smaller the index
    //Chih-Yuan Yang: This is my own function in the utiliti_compute.cpp
    return SortIndex(distances, true);
/*    
    vector<int> index_vector(distances.size(), 0);
    for (unsigned int i = 0 ; i != index_vector.size() ; i++) {
        index_vector[i] = i;
    }
    sort(index_vector.begin(), index_vector.end(),
        [&](const int& a, const int& b) {
            return (distances[a] > distances[b]);
        }
    );
    return index_vector;
    */
}

//Sort OpenVINO's openpose results by the distance between neck to nose, eyes, and ears
vector<HumanPose> SortPosesByHeight(const vector<HumanPose>& poses)
{
    //compute the distnaces
    vector<float> distances(poses.size(),0);
    for( unsigned int idx = 0; idx < poses.size(); idx++ )
    {
        HumanPose pose = poses.at(idx);
        Rect region = GetPoseRegion(pose);
        distances[idx] = region.height;
    }

    //get sorted indexes, the larger distance, the smaller the index
    vector<int> index_order = SortIndex(distances, true);
    vector<HumanPose> result;
    for( unsigned int idx = 0; idx < poses.size(); idx++ )
    {
        result.push_back(poses.at(index_order.at(idx)));
    }
    return result;
}

/*
vector<array<float, 1536>> ConvertPoseRegionsToReIDFeatures(
    const vector<PoseRegion>& pairs, 
    PSE& id_feature_generator,
    unique_ptr<Session>& tf_session)
{
    vector<array<float, 1536>> return_vector;
    const Vec3f image_mean = Vec3f(105.69332121, 99.12930469, 97.90910844);
    const Vec3f image_std = Vec3f(50.26135204, 48.62204008, 48.24029389);   //the scale here seems 0~255
    for(unsigned int i = 0 ; i < pairs.size() ; i++)
    {
        Tensor tensor_image = ConvertMatToNormalizedTensor(pairs[i].mat, image_mean, image_std);
        const int height = pairs[i].mat.rows;
        const int width = pairs[i].mat.cols;
        const int depth = 18;
        Tensor tensor_posemap(DT_FLOAT, TensorShape({1,height,width,depth}));
        ConvertPoseToTensor(pairs[i].pose, tensor_posemap);
        Tensor ConcatenatedTensor = ConcatenateTensors(tensor_image, tensor_posemap, 3);
        //resize tensor to 224x224
        Tensor resized_Tensor = ResizeTensor(ConcatenatedTensor, 224, 224);
        array<float, 1536> feature = id_feature_generator.ComputePSN_Feature(resized_Tensor, tf_session);
        return_vector.push_back(feature);
    }
    return return_vector;
}
*/