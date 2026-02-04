#!/bin/bash
#Author: Chih-Yuan Yang
#2026 Feb 4
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
    #Cannot use 4b. It will use CPU to run the model
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:1b \
        --stage 0 \
        --ImageSaveEveryNFrame 1 \
        --Language Chinese \
        --DefaultSaveImage false
elif [[ "$1" = "4080" ]]; then
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:4b \
        --stage 0 \
        --ImageSaveEveryNFrame 1 \
        --Language Chinese \
        --DefaultSaveImage false
elif [[ "$1" = "4090" ]]; then
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:12b \
        --ImageSaveEveryNFrame 1 \
        --Language Chinese \
        --DefaultSaveImage false
elif [[ "$1" = "AGXOrin" ]]; then

    # Set GStreamer to prefer the avdec_h264 decoder for better performance on Orin
    export GST_PLUGIN_FEATURE_RANK=avdec_h264:MAX

    # 1. Switch the HDA card to HDMI mode
    # We use the specific name found in your log
    pactl set-card-profile alsa_card.platform-3510000.hda output:hdmi-stereo || true

    # 2. Find the correct HDMI Sink name
    # This search for the device associated with card 0 (the HDA card)
    HDMI_SINK=$(pactl list short sinks | grep "3510000.hda" | awk '{print $2}')

    # 3. Set it as default
    if [ ! -z "$HDMI_SINK" ]; then
        pactl set-default-sink "$HDMI_SINK"
        echo "Successfully switched to HDMI Sink: $HDMI_SINK"
    else
        echo "Error: HDMI Sink not found after profile switch."
    fi

    # 4. Run our program
    build/RobotNurseHelper \
        --WhisperModel "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        --ImageSaveDirectory "$HOME/Downloads/raw_images" \
        --LanguageModel gemma3:4b \
        --ImageSaveEveryNFrame 1 \
        --Language Chinese \
        --DefaultSaveImage false
        
elif [[ "$1" = "debug" ]]; then
    # Ensure arguments are correctly passed to gdb via --args
    gdb --args build/RobotNurseHelper \
        "--WhisperModel" "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin" \
        "--ImageSaveDirectory" "$HOME/Downloads/raw_images" \
        "--LanguageModel" "gemma3:1b" \
        "--stage" "0" \
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