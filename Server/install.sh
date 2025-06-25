#!/bin/bash

#2025/6/1
#Install Robot Nurse Helper to Ubuntu 24.04
#Author: Chih-Yuan Yang
#Project: Robot Nurse Helper

read -p "How many Gb VRAM does your machine have?? [0/2/4/8/16/24]" VRAMSize
read -p "What is the robot model you use? [Zenbo/Kebbi/ZenboJrII]" RobotModel

#Check if the VARAM size is valid
allowed_numbers=(0 2 4 8 16 24)

# Validate if the input is a valid integer
if ! [[ "$VRAMSize" =~ ^[0-9]+$ ]]; then
  echo "Error: '$VRAMSize' is not a valid integer. Please try again."
  exit
fi

# Check if the input number is in the allowed_numbers array
is_valid=false
for allowed_val in "${allowed_numbers[@]}"; do
  if [ "$VRAMSize" -eq "$allowed_val" ]; then
    is_valid=true
    break # Found a match, no need to check further
  fi
done

if ! [[ "$is_valid" = true ]]; then
  echo "Error: '$VRAMSize' is not in the allowed list. Please try again."
fi

#Check if the RobotModel is valid
allowed_robot_models=("Zenbo" "Kebbi" "ZenboJrII")
# Validate if the input is a valid string
if ! [[ "$RobotModel" =~ ^[A-Za-z]+$ ]]; then
  echo "Error: '$RobotModel' is not a valid string. Please try again."
  exit
fi
# Check if the input string is in the allowed_robot_models array
is_valid_robot_model=false
for allowed_robot_model in "${allowed_robot_models[@]}"; do
  if [ "$RobotModel" == "$allowed_robot_model" ]; then
    is_valid_robot_model=true
    break # Found a match, no need to check further
  fi
done
if ! [[ "$is_valid_robot_model" = true ]]; then
  echo "Error: '$RobotModel' is not in the allowed list. Please try again."
  exit
fi

if (( VRAMSize > 0 )); then
  echo "We will detect the GPU driver. If there is no driver, we will install the driver for you. But you need to restart your PC after the installation."
  #Check if the GPU driver is installed
  sudo apt update   #this command is required because Ubuntu's repositories URL changed after its release in 2024 April.
  ubuntu-drivers devices
  sudo ubuntu-drivers autoinstall
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
if [ -d "RobotNurseHelper_build" ]; then
    rm -rf RobotNurseHelper_build
fi
mkdir RobotNurseHelper_build

#install OpenCV 4.11, which is required by MediaPipe
#install OpenCV 4.11 first, because it requires to key in sudo password again
cd ~/RobotNurseHelper_build
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


#intall protobuf 3.19.1
cd ~/RobotNurseHelper_build
wget -O protobuf-all-3.19.1.zip https://github.com/protocolbuffers/protobuf/releases/download/v3.19.1/protobuf-all-3.19.1.zip
unzip protobuf-all-3.19.1.zip
cd ~/RobotNurseHelper_build/protobuf-3.19.1
./configure
#make -j $(nproc)   #why only 1 core is used?
make -j 10    #prevent memory peak usage
make check     # this command will generate a peak memory usage
sudo make install
sudo ldconfig # refresh shared library cache.



#install MediaPipe v0.10.22
cd ~
if [ -d "mediapipe" ]; then
    rm -rf mediapipe
fi
git clone https://github.com/google-ai-edge/mediapipe.git
cd mediapipe
git checkout v0.10.22

#download our files
cd 
if [ -d "RobotNurseHelper" ]; then
    rm -rf RobotNurseHelper
fi
git clone https://github.com/yangchihyuan/RobotNurseHelper.git
#copy our code to the mediapipe folder
cp -r ~/RobotNurseHelper/Server/mediapipe_addition/* ~/mediapipe/

#Install bazelisk
cd ~/RobotNurseHelper_build
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
cd ~/RobotNurseHelper_build
if [ -d "portaudio" ]; then
    rm -rf portaudio
fi
git clone https://github.com/PortAudio/portaudio.git

#There is an instruction page teaching how to compile and install PortAudio (Link) However, as the page claims it is not reviewed, we modified its commands to

sudo apt-get -y install libasound2-dev
cd ~/RobotNurseHelper_build/portaudio
./configure
make -j $(nproc)
sudo make install

#On some Linux systems, we need to reload the system to make the library visible.
sudo ldconfig

#whisper.cpp
#It is voice-to-text library and we utilize it on our server-side program to quickly generate sentences spoken by an operator, which will be sent to the robot to speak out. There is no package make for the Ubuntu system, and we need to compile it from it source file downloaded from its GitHub repository

#Debug info 25/3/18,whisper.cpp v1.7.5 changes its install commands
cd ~/RobotNurseHelper_build
if [ -d "whisper.cpp" ]; then
    rm -rf whisper.cpp
fi
git clone https://github.com/ggerganov/whisper.cpp.git
cd ~/RobotNurseHelper_build/whisper.cpp
git checkout v1.7.5
if ((VRAMSize==0)); then
  bash ./models/download-ggml-model.sh tiny
  cmake -B build
  cmake --build build -j10 --config Release
else
  sudo apt -y install nvidia-cuda-toolkit
  if ((VRAMSize==2));
    bash ./models/download-ggml-model.sh small
  else
    bash ./models/download-ggml-model.sh large-v3-turbo
  fi
  cmake -B build -DGGML_CUDA=1
  cmake --build build -j10 --config Release    #Don't use -j, there are 20 cores in my laptop, which will cause a peak memory usage
fi

#onnx
cd ~/RobotNurseHelper_build
wget -O onnxruntime-linux-x64-1.12.1.tgz https://github.com/microsoft/onnxruntime/releases/download/v1.12.1/onnxruntime-linux-x64-1.12.1.tgz
tar -xvzf onnxruntime-linux-x64-1.12.1.tgz

#silero-vad
cd ~/RobotNurseHelper_build
git clone https://github.com/snakers4/silero-vad.git

#ollama
sudo snap install curl
cd ~/RobotNurseHelper_build/
curl -fsSL https://ollama.com/install.sh | sh
if((VRAMSize<=2)); then
  ollama pull gemma3:1b
else
  ollama pull gemma3:4b
fi


#ollama-hpp
cd ~/RobotNurseHelper_build
git clone https://github.com/jmont-dev/ollama-hpp.git

#Build our own program
cd ~/RobotNurseHelper/Server
./build_project.sh $RobotModel
#copy the required mediapipe files to Server
cp -r ~/mediapipe/bazel-bin/mediapipe/examples/desktop/libmp/libmp_gpu.so.runfiles/mediapipe/mediapipe .
if [ -d "temp" ]; then
    rm -rf temp
fi
mkdir temp
# this file mediapipe/modules/hand_landmark/handedness.txt is required to run holistic trackiing
find ~/mediapipe/mediapipe -type f \( -name "*.txt" \) -exec cp --parents {} temp \;
cp -r temp/home/$USER/mediapipe/mediapipe .
rm -rf temp

#copy the file to prevent Nvidia GPU from being unavailable after laptop suspends
sudo cp ~/RobotNurseHelper/Server/nvidia-power-management.conf /etc/modprobe.d/
sudo update-initramfs -u