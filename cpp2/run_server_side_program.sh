#!/bin/bash
#Author: Chih-Yuan Yang
#2025/5/1

if [ $# == 0 ]; then
    build/ZenboNurseHelper -m $HOME/ZenboNurseHelper_build/whisper.cpp/models/ggml-base.bin -d $HOME/Downloads
elif [ $1 == "turbo" ]; then
    build/ZenboNurseHelper -m $HOME/ZenboNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin -d $HOME/Downloads
elif [ $1 == "debug" ]; then
    gdb --args build/ZenboNurseHelper "-m" "$HOME/ZenboNurseHelper_build/whisper.cpp/models/ggml-base.bin" "-d" "$HOME/Downloads"
elif [ $1 == "valgrind" ]; then
    valgrind build/ZenboNurseHelper
fi
