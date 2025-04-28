#!/bin/bash
#Chih-Yuan Yang 2025
#I wrote this shell script file to call another shell script file.

if [ $# == 0 ]; then
    cmake --build build -j $(nproc)
elif [ $1 == "fresh" ]; then
    rm -rf build
    cmake -S . -B build
    cmake --build build -j $(nproc)
fi
