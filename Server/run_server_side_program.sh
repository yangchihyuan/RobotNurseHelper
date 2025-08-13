#!/bin/bash
#Author: Chih-Yuan Yang
#2025/7/29
# Exit immediately if a command exits with a non-zero status.
set -e

# Always quote variables in conditionals
if [[ "$#" -eq 0 ]]; then # Use -eq for numerical comparison for $#
    echo "Please specify the GPU model as an argument."
    exit 1 # Exit with a non-zero status to indicate an error
elif [[ "$1" = "3050" ]]; then # Use = for string comparison and quote $1
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:1b \
        --previous_context \
        --ImageSaveEveryNFrame 5 \
        --Language Chinese \
        --DefaultSaveImage true
elif [[ "$1" = "4070" ]]; then
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:1b \
        --ImageSaveEveryNFrame 1 \
        --Language Chinese \
        --DefaultSaveImage false
elif [[ "$1" = "4080" ]]; then
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:4b \
        --stage 2 \
        --ImageSaveEveryNFrame 1 \
        --Language Chinese \
        --DefaultSaveImage false
elif [[ "$1" = "4090" ]]; then
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:12b \
        --stage 2 \
        --ImageSaveEveryNFrame 1 \
        --Language English \
        --DefaultSaveImage false
elif [[ "$1" = "exp2" ]]; then
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:12b \
        --stage 0 \
        --ImageSaveEveryNFrame 1 \
        --Language Chinese \
        --DefaultSaveImage false

elif [[ "$1" = "cyy" ]]; then
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:4b \
        --stage 0 \
        --ImageSaveEveryNFrame 1 \
        --Language Chinese \
        --DefaultSaveImage false
elif [[ "$1" = "debug" ]]; then
    # Ensure arguments are correctly passed to gdb via --args
    gdb --args build/RobotNurseHelper \
        "--WhisperModel" "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-base.bin" \
        "--ImageSaveDirectory" "$HOME/Downloads/raw_images" \
        "--LanguageModel" "gemma3:4b" \
        "--ImageSaveEveryNFrame" "5" \
        "--Language" "Chinese" \
        "--DefaultSaveImage" "false" 
elif [[ "$1" = "valgrind" ]]; then
    valgrind build/RobotNurseHelper # Consider adding specific valgrind flags if needed, e.g., --leak-check=full
else 
    echo "Invalid GPU model specified. Please use '3050', '4070', '4090', 'debug', or 'valgrind'."
    exit 1 # Exit with a non-zero status
fi

exit 0 # Indicate successful execution