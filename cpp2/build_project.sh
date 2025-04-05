#!/bin/bash
#Chih-Yuan Yang 2025
#I wrote this shell script file to call another shell script file.

if [ $# == 0 ]; then
elif [ $1 == "fresh" ]; then
    rm -rf build
    cmake -S . -B build
fi
cmake --build build -j $(nproc)
