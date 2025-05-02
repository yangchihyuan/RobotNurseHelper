#!/bin/bash

#2025/4/18
#Install Zenbo Nurse Helper cpp2 to Ubuntu 24.04
#Author: Chih-Yuan Yang
#Project: Zenbo Nurse Helper

read -p "Does your PC have an Nvidia GPU RTX 30 or 40 serial? [y/n]" response

if [[ "$response" =~ ^[yYnN]$ ]]; then
  if [[ "$response" =~ ^[yY]$ ]]; then
    NVidiaGPURTXavailable="y"
    echo "We will detect the GPU driver. If there is no driver, we will install the driver for you. But you need to restart your PC after the installation."
    #Check if the GPU driver is installed
    ubuntu-drivers devices
    sudo ubuntu-drivers autoinstall
  else
    NVidiaGPURTXavailable="n"
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

#create an empty workding directory
if [ -d "ZenboNurseHelper_build" ]; then
    rm -rf ZenboNurseHelper_build
fi
mkdir ZenboNurseHelper_build


#intall protobuf 3.19.1
cd ~/ZenboNurseHelper_build
wget -O protobuf-all-3.19.1.zip https://github.com/protocolbuffers/protobuf/releases/download/v3.19.1/protobuf-all-3.19.1.zip
unzip protobuf-all-3.19.1.zip
cd ~/ZenboNurseHelper_build/protobuf-3.19.1
./configure
make -j $(nproc)   #why only 1 core is used?
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
if [ -d "mediapipe" ]; then
    rm -rf mediapipe
fi
git clone https://github.com/google-ai-edge/mediapipe.git
cd mediapipe
git checkout v0.10.22

#download our files
cd ~ 
if [ -d "ZenboNurseHelper" ]; then
    rm -rf ZenboNurseHelper
fi
git clone https://github.com/yangchihyuan/ZenboNurseHelper.git
#copy our code to the mediapipe folder
cp -r ~/ZenboNurseHelper/cpp2/mediapipe_addition/* ~/mediapipe/

#Install bazelisk
cd ~/ZenboNurseHelper_build
wget -O bazelisk-amd64.deb https://github.com/bazelbuild/bazelisk/releases/download/v1.25.0/bazelisk-amd64.deb
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
cd ~/ZenboNurseHelper_build
if [ -d "portaudio" ]; then
    rm -rf portaudio
fi
git clone https://github.com/PortAudio/portaudio.git

#There is an instruction page teaching how to compile and install PortAudio (Link) However, as the page claims it is not reviewed, we modified its commands to

sudo apt-get -y install libasound2-dev
cd ~/ZenboNurseHelper_build/portaudio
./configure
make -j $(nproc)
sudo make install

#On some Linux systems, we need to reload the system to make the library visible.
sudo ldconfig

#whisper.cpp
#It is voice-to-text library and we utilize it on our server-side program to quickly generate sentences spoken by an operator, which will be sent to the Zenbo robot to speak out. There is no package make for the Ubuntu system, and we need to compile it from it source file downloaded from its GitHub repository

#Debug info 25/3/18,whisper.cpp v1.7.5 changes its install commands
cd ~/ZenboNurseHelper_build
if [ -d "whisper.cpp" ]; then
    rm -rf whisper.cpp
fi
git clone https://github.com/ggerganov/whisper.cpp.git
cd ~/ZenboNurseHelper_build/whisper.cpp
git checkout v1.7.5
bash ./models/download-ggml-model.sh base
if [ "$NVidiaGPURTXavailable" == "n" ]; then
    #This is the CPU mode
    #It will download ggml-base.bin from the HuggingFace website.
    cmake -B build
    cmake --build build --config Release
else
    #This is the NVidia 4070 mode
    bash ./models/download-ggml-model.sh large-v3-turbo
    sudo apt -y install nvidia-cuda-toolkit
    cmake -B build -DGGML_CUDA=1
    cmake --build build --config Release    # I cannot use -j here. It will run out my 16G RAM + 4G Swap and cause errors.
fi

#Build our own program
cd ~/ZenboNurseHelper/cpp2
./build_project.sh fresh
#copy the required mediapipe files to cpp2
cp -r ~/mediapipe/bazel-bin/mediapipe/examples/desktop/libmp/libmp_gpu.so.runfiles/mediapipe/mediapipe .
if [ -d "temp" ]; then
    rm -rf temp
fi
mkdir temp
# this file mediapipe/modules/hand_landmark/handedness.txt is required to run holistic trackiing
find ~/mediapipe/mediapipe -type f \( -name "*.txt" \) -exec cp --parents {} temp \;
cp -r temp/home/$USER/mediapipe/mediapipe .
rm -rf temp