#!/bin/bash
#Chih-Yuan Yang 2025
#I wrote this shell script file to call another shell script file.

cd build
cmake ..
cd ..
cmake --build build
