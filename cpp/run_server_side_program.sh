#!/bin/bash
#Chih-Yuan Yang 2024

if [ $# == 0 ]; then
export QT_NO_FT_CACHE=1
~/omz_demos_build/intel64/Release/9_NurseHelper \
--pose_model ~/open_model_zoo/models/intel/human-pose-estimation-0001/FP32/human-pose-estimation-0001.xml \
--SaveTransmittedImage=true \
--save_to_directory=/home/chihyuan/Downloads \
--port_number=8895
elif [ $1 == "debug" ]; then
export QT_NO_FT_CACHE=1
gdb --args ~/omz_demos_build/intel64/Release/9_NurseHelper \
--pose_model ~/open_model_zoo/models/intel/human-pose-estimation-0001/FP32/human-pose-estimation-0001.xml \
--SaveTransmittedImage=true \
--save_to_directory=/home/chihyuan/Downloads \
--port_number=8895
elif [ $1 == "valgrind" ]; then
export QT_NO_FT_CACHE=1
valgrind ~/omz_demos_build/intel64/Release/9_NurseHelper \
--pose_model ~/open_model_zoo/models/intel/human-pose-estimation-0001/FP32/human-pose-estimation-0001.xml \
--SaveTransmittedImage=true \
--save_to_directory=/home/chihyuan/Downloads \
--port_number=8895
fi

