#!/bin/bash
#Author: Chih-Yuan Yang
#2025/3/12

if [ $# == 0 ]; then
    build/ZenboNurseHelper
elif [ $1 == "debug" ]; then
    gdb build/ZenboNurseHelper
elif [ $1 == "valgrind" ]; then
    valgrind build/ZenboNurseHelper
fi
