#!/bin/bash
#Author: Chih-Yuan Yang
#2026 Feb 25
# Exit immediately if a command exits with a non-zero status.
set -e

# Change to the directory of the script to ensure relative paths work correctly
cd "$(dirname "$0")"

# Always quote variables in conditionals
if [[ "$#" -eq 0 ]]; then # Use -eq for numerical comparison for $#
    build/RobotNurseHelper
elif [[ "$1" = "AGXOrin" ]]; then

    # Set GStreamer to prefer the avdec_h264 and h265 decoder for better performance on Orin
    export GST_PLUGIN_FEATURE_RANK=avdec_h264:MAX,avdec_h265:MAX,nvv4l2h264dec:NONE,nvv4l2h265dec:NONE

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
    build/RobotNurseHelper
        
elif [[ "$1" = "debug" ]]; then
    # Ensure arguments are correctly passed to gdb via --args
    gdb --args build/RobotNurseHelper
elif [[ "$1" = "valgrind" ]]; then
    valgrind build/RobotNurseHelper # Consider adding specific valgrind flags if needed, e.g., --leak-check=full
else 
    echo "Error: Invalid argument specified. If the program is running on a PC, please ignore the argument. Otherwise, please use 'AGXOrin', 'debug', or 'valgrind'."
    exit 1 # Exit with a non-zero status
fi

exit 0 # Indicate successful execution