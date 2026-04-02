#!/bin/bash
#Author: Chih-Yuan Yang
#2026 April 1
# Exit immediately if a command exits with a non-zero status.
set -e

# Change to the directory of the script to ensure relative paths work correctly
cd "$(dirname "$0")"

# Always quote variables in conditionals
if [[ "$1" == "-h" || "$1" == "--help" || "$#" -gt 1 ]]; then
    echo "Usage: $0 [Setting_file.json | debug | valgrind]"
    echo ""
    echo "Options:"
    echo "  (none)       Runs with the default setting file (json/Setting.json)"
    echo "  <file_path>  Runs with the specified JSON setting file, for example: $0 json/AGXOrin2.json"
    echo "  debug        Runs the program using gdb for debugging"
    echo "  valgrind     Runs the program using valgrind to check for memory leaks"
    exit 1
elif [[ "$#" -eq 0 ]]; then # Use -eq for numerical comparison for $#
    # Set the default Setting.json path if no arguments are provided
    Setting_file="json/Setting.json"    
elif [[ "$1" = "debug" ]]; then             #TODO: I haven't update this debug command to use the new --SettingFile argument, so it still defaults to json/Setting.json. You may want to update this to allow passing a custom Setting file in debug mode as well.
    Setting_file="json/Setting.json"    
    # Ensure arguments are correctly passed to gdb via --args
    gdb --args build/RobotNurseHelper
elif [[ "$1" = "valgrind" ]]; then
    valgrind build/RobotNurseHelper # Consider adding specific valgrind flags if needed, e.g., --leak-check=full
else 
    Setting_file="$1"
fi

#extract the machine field from the Setting JSON file.
clean_json=$(sed 's|//.*||g' "$Setting_file")      # Remove comments from JSON file
Machine=$(echo "$clean_json" | jq -r '.Machine')
echo "The machine is $Machine"

if [[ "$Machine" = "AGXOrin" ]]; then

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
fi

echo "Starting RobotNurseHelper with Setting file: $Setting_file"
build/RobotNurseHelper --SettingFile "$Setting_file"

exit 0 # Indicate successful execution