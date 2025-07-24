#!/bin/bash
#Chih-Yuan Yang 2025/7/23
#Build the Robot Nurse Helper project with CMake
#I wrote this shell script file to call another shell script file.

if [ $# == 1 ]; then
    if [[ "$1" == "fresh" ]]; then
        rm -rf build
        cmake -S . -B build
    elif [[ "$1" == "Zenbo" || "$1" == "ZenboJrII" ]]; then
        echo "Building for Zenbo or ZenboJrII"
        rm -rf build
        cmake -S . -B build -DROBOT_MODEL=Zenbo
    elif [[ "$1" == "Kebbi" ]]; then
        echo "Building for Kebbi"
        rm -rf build
        cmake -S . -B build -DROBOT_MODEL=Kebbi
    fi
fi
cmake --build build -j $(nproc)
