#!/bin/bash
#Author: Chih-Yuan Yang
#2025/3/12

if [ $# == 0 ]; then
    build/ZenboNurseHelper -m $HOME/whisper.cpp/models/ggml-base.bin -d $HOME/Downloads
elif [ $1 == "debug" ]; then
    gdb --args build/ZenboNurseHelper "-m" "$HOME/whisper.cpp/models/ggml-base.bin" "-d" "$HOME/Downloads"
elif [ $1 == "valgrind" ]; then
    valgrind build/ZenboNurseHelper
fi
