#!/bin/bash
#Author: Chih-Yuan Yang
#2025/5/25

if [ $# == 0 ]; then
    echo "Please specify the GPU model as an argument."
elif [ $1 == "3050" ]; then
    build/RobotNurseHelper --WhisperModel $HOME/ZenboNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin \
    --ImageSaveDirectory $HOME/Downloads --LanguageModel gemma3:1b \
    --ImageSaveEveryNFrame 5
elif [ $1 == "4070" ]; then
    build/RobotNurseHelper --WhisperModel $HOME/ZenboNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin \
    --ImageSaveDirectory $HOME/Downloads --LanguageModel gemma3:4b \
    --ImageSaveEveryNFrame 1
elif [ $1 == "debug" ]; then
    gdb --args build/RobotNurseHelper "--WhisperModel" "$HOME/ZenboNurseHelper_build/whisper.cpp/models/ggml-base.bin" \
    "--ImageSaveDirectory" "$HOME/Downloads" "--LanguageModel" "gemma3:1b" \
    "--ImageSaveEveryNFrame" "5"
elif [ $1 == "valgrind" ]; then
    valgrind build/RobotNurseHelper
fi
