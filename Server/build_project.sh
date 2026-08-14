#!/bin/bash
#Chih-Yuan Yang 2026/8/03
#Build the Robot Nurse Helper project with CMake
#I wrote this shell script file to call another shell script file.

if [ $# == 1 ]; then
    if [[ "$1" == "clean" ]]; then
        rm -rf build
    elif [[ "$1" == "Zenbo" || "$1" == "ZenboJrII" ]]; then
        echo "Building for Zenbo or ZenboJrII"
        cmake -S . -B build -DROBOT_MODEL=Zenbo
        cmake --build build -j $(nproc)
    elif [[ "$1" == "Kebbi" ]]; then
        echo "Building for Kebbi"
        cmake -S . -B build -DROBOT_MODEL=Kebbi -DCMAKE_CXX_FLAGS="-Wno-psabi"
        cmake --build build -j $(nproc)
    fi
else
    echo "Usage: ./build_project.sh [clean|Zenbo|ZenboJrII|Kebbi]"
fi
