#!/bin/bash

#2025/4/18
#Install Zenbo Nurse Helper cpp2 to Ubuntu 24.04
#Author: Chih-Yuan Yang
#Project: Zenbo Nurse Helper

read -p "Does your PC have an Nvidia GPU 40 serial? [y/n]" response

if [[ "$response" =~ ^[yYnN]$ ]]; then
  if [[ "$response" =~ ^[yY]$ ]]; then
    GPU40available="y"
  else
    GPU40available="n"
  fi
else
  echo "Invalid response. Please enter y, Y, n, or N."
  exit
fi

#Install the compiler
sudo apt -y install build-essential

#install git
sudo apt -y install git

#install zip
sudo apt -y install zip

#install libgtk2.0-dev, which is used in OpenCV to show images
sudo apt -y install libgtk2.0-dev 

#intall protobuf 3.19.1
cd ~/Downloads
wget -O protobuf-all-3.19.1.zip https://github.com/protocolbuffers/protobuf/releases/download/v3.19.1/protobuf-all-3.19.1.zip
unzip protobuf-all-3.19.1.zip
cd ~/Downloads/protobuf-3.19.1
./configure
make -j $(nproc)
make check     # this command will generate a peak memory usage
sudo make install
sudo ldconfig # refresh shared library cache.


#install OpenCV 4.11, which is required by MediaPipe
sudo apt -y install cmake
wget -O opencv4.11.zip https://github.com/opencv/opencv/archive/refs/tags/4.11.0.zip
wget -O opencv_contrib4.11.zip https://github.com/opencv/opencv_contrib/archive/refs/tags/4.11.0.zip
unzip opencv4.11.zip
unzip opencv_contrib4.11.zip
cd opencv-4.11.0
mkdir -p build && cd build
cmake -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.11.0/modules ..
cmake --build . -j $(nproc)
sudo make install
#to config the loading directories to let /usr/local/lib works
sudo ldconfig

#install MediaPipe v0.10.22
cd ~
git clone https://github.com/google-ai-edge/mediapipe.git
cd mediapipe
git checkout v0.10.22

#download our files
cd ~ 
git clone https://github.com/yangchihyuan/ZenboNurseHelper.git
#copy our code to the mediapipe folder
cp -r ~/ZenboNurseHelper/cpp2/mediapipe_addition/* ~/mediapipe/

#Install bazelisk
wget https://github.com/bazelbuild/bazelisk/releases/download/v1.25.0/bazelisk-amd64.deb
sudo dpkg -i bazelisk-amd64.deb

#install OpenGL libraries, which will be used in MediaPipe for compiling GPU-related code.
#This is MediaPipe's requirement
sudo apt-get -y install mesa-common-dev libegl1-mesa-dev libgles2-mesa-dev

#build libmp library
cd ~/mediapipe
bazel build -c opt mediapipe/examples/desktop/libmp:libmp_gpu.so

#Qt
#We use it to create our GUI
sudo apt -y install qt6-base-dev    
sudo apt -y install qt6-multimedia-dev
#It will install Qt version 6.4.2.

#Hint
#The two commands to install Qt base and multimedia libraries allow you to compile this project. However, they do not install Qt Designer, a convenient tool to the GUI file mainwindow.ui. If you want to install Qt Designer, you need to use this command
sudo apt -y install qtcreator
#The Qt creator takes more than 1G disk space because it requires many libraries. Once installed, you can launch the program to open the mainwindow.ui file with Qt Designer.

#PortAudio
#We use it to play voice on the server transmitted from the Android app and received from the robot's microphone. There is no package made for the Ubuntu system, and we need to compile it from downloaded source files, which are available on its GitHub page
cd ~
git clone https://github.com/PortAudio/portaudio.git

#There is an instruction page teaching how to compile and install PortAudio (Link) However, as the page claims it is not reviewed, we modified its commands to

sudo apt-get -y install libasound2-dev
cd ~/portaudio
./configure
make -j $(nproc)
sudo make install

#On some Linux systems, we need to reload the system to make the library visible.
sudo ldconfig

#whisper.cpp
#It is voice-to-text library and we utilize it on our server-side program to quickly generate sentences spoken by an operator, which will be sent to the Zenbo robot to speak out. There is no package make for the Ubuntu system, and we need to compile it from it source file downloaded from its GitHub repository

#Debug info 25/3/18,whisper.cpp v1.7.5 changes its install commands
cd ~
git clone https://github.com/ggerganov/whisper.cpp.git
cd ~/whisper.cpp
git checkout v1.7.5
if [ "$GPU40available" == "n" ]; then
    #This is the CPU mode
    #It will download ggml-base.bin from the HuggingFace website.
    bash ./models/download-ggml-model.sh base
    cmake -B build
    cmake --build build --config Release
else
    #This is the NVidia 4070 mode
    bash ./models/download-ggml-model.sh large-v3-turbo
    cmake -B build -DGGML_CUDA=1
    cmake --build build -j --config Release
fi

#Build our own program
cd ~/ZenboNurseHelper/cpp2
./build_project.sh fresh

