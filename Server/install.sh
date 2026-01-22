#!/bin/bash

#2025/7/23
#Install Robot Nurse Helper to Ubuntu 24.04
#Author: Chih-Yuan Yang
#Project: Robot Nurse Helper

read -p "Is your secure boot off in your motherboard's UEFI setting? [y/n]" SecureBoot
if ! [[ "$SecureBoot" == "Y" || "$SecureBoot" == "y" ]]; then
  echo "This install.sh script cannot fully automatically install a Nvidia GPU driver because your UEFI secure boot is on. If you want to use this instal.sh to fully automatically install a NVidia GPU driver, you need to set your UEFI secure boot off. Otherwise, you need to install the Nvidia GPU driver manually."
  read -p "Do you want to stop the install.sh? [y/n]" StopInstall
  if ! [[ "$StopInstall" == "Y" || "$StopInstall" == "y" ]]; then
    echo "You can continue to install the Robot Nurse Helper, but you need to install the Nvidia GPU driver manually after the installation."
  else
    echo "You can run this install.sh again after you set your UEFI secure boot off."
    exit  #stop the installation script
  fi  
fi

read -p "What is your GPU model? [none/3050laptop/4070laptop/4080/4090]" GPUModel
read -p "What is the robot model you use? [Zenbo/Kebbi/ZenboJrII]" RobotModel

#Check if the VARAM size is valid
if [ "$GPUModel" = "none" ]; then
  VRAMSize=0
elif [ "$GPUModel" = "3050laptop" ]; then
  VRAMSize=4
elif [ "$GPUModel" = "4070laptop" ]; then
  VRAMSize=8
elif [ "$GPUModel" = "4080" ]; then
  VRAMSize=16
elif [ "$GPUModel" = "4090" ]; then
  VRAMSize=24
else
  echo "Error: '$GPUModel' is not in the allowed list. Please try again."
  exit
fi

if (( VRAMSize > 0 )); then
  #echo "We will detect the GPU driver. If there is no driver, we will install the driver for you. But you need to restart your PC after the installation."
  #Check if the GPU driver is installed
  sudo apt update   #this command is required because Ubuntu's repositories URL changed after its release in 2024 April.
  #ubuntu-drivers devices             #list available drivers
  #Don't use this command. It sometimes downgrades the GPU driver to an older version, which causes boot-failure problems.
  #sudo ubuntu-drivers autoinstall    #Sometimes the system need a reboot. Otherwise Ubuntu does not detect the GPU.
  nvidia-smi
  read -p "Can you see the nvidia-smi GPU usage messages? [y/n]" GPUDriverWork
  if ! [[ "$GPUDriverWork" == "Y" || "$GPUDriverWork" == "y" ]]; then
    echo "Your NVidia GPU driver is not ready yet. You need to install the NVidia GPU driver first, and then run this install.sh script again."
    exit
  fi

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
#sudo apt -y install cmake
# Ubuntu 22.04 cmake version is 3.22.1, which is not enough for emotiEfflib, which needs cmake 3.29 or above.
# The snap version is 3.31+
sudo snap install cmake --classic

wget -O opencv4.11.zip https://github.com/opencv/opencv/archive/refs/tags/4.11.0.zip
wget -O opencv_contrib4.11.zip https://github.com/opencv/opencv_contrib/archive/refs/tags/4.11.0.zip
unzip opencv4.11.zip
unzip opencv_contrib4.11.zip
cd opencv-4.11.0
mkdir -p build && cd build
sudo apt install libvtk9-dev       #vtk is required to compile opencv_vis module, which is required by EmotiEffLib
cmake  .. -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.11.0/modules \
      -D WITH_VTK=ON \             #for emotiefflib
      -D BUILD_opencv_viz=ON       #for emotiefflib
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
#if I use "make -j $(nproc)", there is a peak of memory usage, which exceeds the RAM+SWAP size on some laptops.
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

#Qt Multimedia Plugins
#We need to install these plugins to let Qt Multimedia module play audio and video files properly
sudo apt install -y gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav

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
#It is a voice-to-text library and we utilize it on our server-side program to quickly generate sentences spoken by an operator, which will be sent to the robot to speak out. There is no package make for the Ubuntu system, and we need to compile it from it source file downloaded from its GitHub repository

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
  if ((VRAMSize==2)); then
    bash ./models/download-ggml-model.sh small
  else
    bash ./models/download-ggml-model.sh large-v3-turbo
  fi
  cmake -B build -DGGML_CUDA=1
  cmake --build build -j10 --config Release    #Don't use -j, there are 20 cores in my laptop, which will cause a peak memory usage
fi

#onnx
cd ~/RobotNurseHelper_build
#wget -O onnxruntime-linux-x64-1.12.1.tgz https://github.com/microsoft/onnxruntime/releases/download/v1.12.1/onnxruntime-linux-x64-1.12.1.tgz
#tar -xvzf onnxruntime-linux-x64-1.12.1.tgz
wget -O onnxruntime-linux-x64-gpu-1.22.0.tgz https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-x64-gpu-1.22.0.tgz
tar -xvzf onnxruntime-linux-x64-gpu-1.22.0.tgz

#silero-v
cd ~/RobotNurseHelper_build
git clone https://github.com/snakers4/silero-vad.git

#The EmotiEffLib uses a 3rd party library 3rdparty/xtl/CMakeLists.txt, which requires CMake 3.29 or above.
sudo apt update
sudo apt install software-properties-common wget apt-transport-https ca-certificates gnupg -y
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
# Example: Replace <UBUNTU_CODENAME> with your actual codename
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
# Update your package list
sudo apt update
sudo apt install cmake -y
cmake --version   #It should be 4.1.2

#EmotiEffLib needs libopenblas-dev
sudo apt install libopenblas-dev

#EmotiEffLib
cd ~/RobotNurseHelper_build
git clone https://github.com/sb-ai-lab/EmotiEffLib.git
cd EmotiEffLib
git submodule update --init --recursive
cd emotieffcpplib
mkdir build && cd build
#I am not sure if this command works. Check it later.
#Their CMakeLists.txt file needs /home/chihyuan/RobotNurseHelper_build/onnxruntime-linux-x64-gpu-1.22.0/lib64, but there is no lib64 folder in onnxruntime-linux-x64-gpu-1.22.0. There is only a lib folder.
#So, I create a symbolic link lib64 to lib
cd ~/RobotNurseHelper_build/onnxruntime-linux-x64-gpu-1.22.0
ln -s lib lib64
cd ~/RobotNurseHelper_build/EmotiEffLib/emotieffcpplib/build
cmake .. -DWITH_ONNX=~/RobotNurseHelper_build/onnxruntime-linux-x64-gpu-1.22.0 -DBUILD_SHARED_LIBS=ON
make -j$(nproc)
#The .so files are in ~/RobotNurseHelper_build/EmotiEffLib/emotieffcpplib/build/lib

#ollama
sudo snap install curl
cd ~/RobotNurseHelper_build/
curl.snap-acked        #ollama changed its installation script. There is a text explanation in the script. It only accepts Snap-curand we need to use this command first
curl -fsSL https://ollama.com/install.sh | sh
if((VRAMSize<=2)); then
  ollama pull gemma3:1b
elif((VRAMSize=12)); then
  ollama pull gemma3:12b
else
  ollama pull gemma3:4b
fi


#ollama-hpp
cd ~/RobotNurseHelper_build
git clone https://github.com/jmont-dev/ollama-hpp.git
cd ~/RobotNurseHelper_build/ollama-hpp
git checkout v0.9.5
#The ollama.hpp vesioin 0.9.7 has a conclict with c++13 std::hash. My previous verion is 0.9.5, does not have this problem.

#dlib library for Facial expression recognition
#The precompiled libdlib-dev does not work it enables the DLIB_NO_GUI_SUPPORT
#sudo apt -y install libdlib-dev       #Ubuntu 24.04 has dlib version 19.24.0-1 available in its repository
cd ~/RobotNurseHelper_build/
#This command will go wrong in the future because new versions will changes its download URL
#The curl program in JetPack 6.2.1 is snap version, which conflicts with SELinux's security configuration.
sudo snap remove curl
sudo apt update && sudo apt install curl -y
#update the path cache
hash -d curl
curl https://dlib.net/files/dlib-20.0.tar.bz2 --output dlib-20.0.tar.bz2
tar -xjvf dlib-20.0.tar.bz2

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