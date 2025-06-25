#!/bin/bash
#Author: Chih-Yuan Yang
#2025/5/25

if [ $# == 0 ]; then
    echo "Please specify the GPU model as an argument."
elif [ $1 == "3050" ]; then
    build/RobotNurseHelper --WhisperModel $HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin \
    --ImageSaveDirectory $HOME/Downloads/raw_images --LanguageModel gemma3:1b \
    --ImageSaveEveryNFrame 5 --Language Chinese --DefaultSaveImage true
elif [ $1 == "4070" ]; then
    build/RobotNurseHelper --WhisperModel $HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin \
    --ImageSaveDirectory $HOME/Downloads/raw_images --LanguageModel gemma3:4b \
    --ImageSaveEveryNFrame 1 --Language Chinese --DefaultSaveImage off
elif [ $1 == "4090" ]; then
    build/RobotNurseHelper --WhisperModel $HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin \
    --ImageSaveDirectory $HOME/Downloads/raw_images --LanguageModel gemma3:12b \
    --ImageSaveEveryNFrame 1 --Language English --DefaultSaveImage off
elif [ $1 == "debug" ]; then
    gdb --args build/RobotNurseHelper "--WhisperModel" "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-base.bin" \
    "--ImageSaveDirectory" "$HOME/Downloads/raw_images" "--LanguageModel" "gemma3:1b" \
    "--ImageSaveEveryNFrame" "5" "--Language" "Chinese" "--DefaultSaveImage" "true"
elif [ $1 == "valgrind" ]; then
    valgrind build/RobotNurseHelper
else 
    echo "Invalid GPU model specified. Please use '3050', '4070', 'debug', or 'valgrind'."
fi
