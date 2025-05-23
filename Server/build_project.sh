#!/bin/bash
#Chih-Yuan Yang 2025
#I wrote this shell script file to call another shell script file.

if [ $1 == "fresh" ]; then
    rm -rf build
    cmake -S . -B build
elif [ $1 == "Zenbo" ]; then
    rm -rf build
    cmake -S . -B build -DROBOT_MODEL=Zenbo
elif [ $1 == "Kebbi" ]; then
    rm -rf build
    cmake -S . -B build -DROBOT_MODEL=Kebbi
fi
cmake --build build -j $(nproc)
