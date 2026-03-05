#!/bin/bash

#2026 Jan 22
#Install Robot Nurse Helper to Ubuntu 24.04 and NVidia AGX Orin
#Author: Chih-Yuan Yang
#Project: Robot Nurse Helper

read -p "Is your RAM + swap greater than 32G? [Y/n]" EnoughRAM
if ! [[ "$EnoughRAM" == "Y" || "$EnoughRAM" == "y" ]]; then
  echo "You need 32G to compile the RobotNurseHelper program. If you don't have enough RAM, enlarge your swap."
  exit
fi


read -p "What is your machine [PC/AGXOrin]" machine
if [ "$machine" = "AGXOrin" ]; then
  VRAMSize=64
elif [ "$machine" = "PC" ]; then
  VRAMSize=0
else
  echo "Error: '$machine' is not in the allowed list. Please try again."
  exit
fi

if [ "$machine" = "PC" ]; then
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

fi

read -p "What is the robot model you use? [Zenbo/Kebbi/ZenboJrII]" RobotModel
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
#On Ubuntu 22.04, the g++ version is 11.4.0, which only support up to C++17. But my code needs C++20.
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

#sudo apt -y install cmake

if [ "$machine" = "PC" ]; then
  # Ubuntu 22.04 cmake version is 3.22.1, which is not enough for emotiEfflib, which needs cmake 3.29 or above.
  sudo snap install cmake --classic   # version 4.2.2 will be installed
elif [ "$machine" = "AGXOrin" ]; then
  #Snap's cmake does not work on AGX Orin because the the AGX Orin Ubuntu does not a complete SELinux system.
  #The SELinux service is initialzed, but required componenets are missing, which leads to a failure of snap's sandbox.
  #sudo snap install cmake --classic   # version 4.2.2 will be installed
  cd ~/RobotNurseHelper_build
  # Download the official Linux installer script
  wget https://github.com/Kitware/CMake/releases/download/v3.31.3/cmake-3.31.3-linux-aarch64.sh

  # Make it executable
  chmod +x cmake-3.31.3-linux-aarch64.sh

  # Run the installer (choose 'y' for license, 'y' for include subdirectory)
  sudo ./cmake-3.31.3-linux-aarch64.sh --prefix=/usr/local --skip-license
fi

#install OpenCV 4.11, which is required by MediaPipe
#install OpenCV 4.11 first, because it requires to key in sudo password again
cd ~/RobotNurseHelper_build

wget -O opencv4.11.zip https://github.com/opencv/opencv/archive/refs/tags/4.11.0.zip
wget -O opencv_contrib4.11.zip https://github.com/opencv/opencv_contrib/archive/refs/tags/4.11.0.zip
unzip opencv4.11.zip
unzip opencv_contrib4.11.zip
cd opencv-4.11.0
mkdir -p build && cd build
sudo apt install libvtk9-dev       #vtk is required to compile opencv_vis module, which is required by EmotiEffLib
#      -D WITH_VTK=ON \             #for emotiefflib
#      -D BUILD_opencv_viz=ON       #for emotiefflib
cmake  .. -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.11.0/modules \
      -D WITH_VTK=ON \
      -D BUILD_opencv_viz=ON
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
#this command only works for amd64 architecture
if [ "$machine" = "PC" ]; then
  wget -O bazelisk-amd64.deb https://github.com/bazelbuild/bazelisk/releases/download/v1.25.0/bazelisk-amd64.deb
  sudo dpkg -i bazelisk-amd64.deb
elif [ "$machine" = "AGXOrin" ]; then
  wget -O bazelisk-arm64.deb https://github.com/bazelbuild/bazelisk/releases/download/v1.25.0/bazelisk-arm64.deb
  sudo dpkg -i bazelisk-arm64.deb
fi


#install OpenGL libraries, which will be used in MediaPipe for compiling GPU-related code.
#This is MediaPipe's requirement
sudo apt-get -y install mesa-common-dev libegl1-mesa-dev libgles2-mesa-dev

#build libmp library
cd ~/mediapipe
#I need to manually change the sha256 value in mediapipe/WORKSPACE if the bazel build command reports a sha256 mismatch error for the AGX Orin case.
if [ "$machine" = "AGXOrin" ]; then
#    name = "KleidiAI",
#    sha256 = "8eeb81ff6bc7ab2de678c0c4a3d18b02c382a5122ac4edc26a3334c858531739",
  sed -i 's/ad37707084a6d4ff41be10cbe8540c75bea057ba79d0de6c367c1bfac6ba0852/8eeb81ff6bc7ab2de678c0c4a3d18b02c382a5122ac4edc26a3334c858531739/g' WORKSPACE
fi
bazel build -c opt mediapipe/examples/desktop/libmp:libmp_gpu.so

#chekc if the bazel build is successful. Sometimes the repository is unavailable and bazel does not work.
DIR="bazel-out"

if [ -d "$DIR" ]; then
  echo "Directory $DIR exists. Mediapipe is built successfully. You can continue to install the Robot Nurse Helper."
else
  echo "Error: Directory $DIR does not exist. There is a problem with bazel build. Please check the error messages above and fix the problem. You can try to run the bazel build command again after fixing the problem."
  exit
fi

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
  if [ "$machine" = "PC" ]; then
    sudo apt -y install nvidia-cuda-toolkit
  fi
  if ((VRAMSize==2)); then
    bash ./models/download-ggml-model.sh small
  else
    bash ./models/download-ggml-model.sh large-v3-turbo
  fi
  if [ "$machine" = "AGXOrin" ]; then
    #set up the ncvv path
    export PATH=/usr/local/cuda/bin:$PATH
    export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH  
  fi
  cmake -B build -DGGML_CUDA=1
  cmake --build build -j10 --config Release    #Don't use -j, there are 20 cores in my laptop, which will cause a peak memory usage
fi

#onnx
cd ~/RobotNurseHelper_build
if [ "$machine" = "PC" ]; then
  wget -O onnxruntime-linux-x64-gpu-1.22.0.tgz https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-x64-gpu-1.22.0.tgz
  tar -xvzf onnxruntime-linux-x64-gpu-1.22.0.tgz
elif [ "$machine" = "AGXOrin" ]; then
  wget -O onnxruntime-linux-aarch64-1.22.0.tgz https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-aarch64-1.22.0.tgz
  tar -xvzf onnxruntime-linux-aarch64-1.22.0.tgz
fi

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
if [ "$machine" = "PC" ]; then
  cd ~/RobotNurseHelper_build/onnxruntime-linux-x64-gpu-1.22.0
  ln -s lib lib64
  cd ~/RobotNurseHelper_build/EmotiEffLib/emotieffcpplib/build
  cmake .. -DWITH_ONNX=~/RobotNurseHelper_build/onnxruntime-linux-x64-gpu-1.22.0 -DBUILD_SHARED_LIBS=ON
  make -j$(nproc)
  #The .so files are in ~/RobotNurseHelper_build/EmotiEffLib/emotieffcpplib/build/lib
elif [ "$machine" = "AGXOrin" ]; then
  cd ~/RobotNurseHelper_build/onnxruntime-linux-aarch64-1.22.0
  ln -s lib lib64
  cd ~/RobotNurseHelper_build/EmotiEffLib/emotieffcpplib/build
  cmake .. -DWITH_ONNX=~/RobotNurseHelper_build/onnxruntime-linux-aarch64-1.22.0 -DBUILD_SHARED_LIBS=ON
  make -j$(nproc)
  #The .so files are in ~/RobotNurseHelper_build/EmotiEffLib/emotieffcpplib/build/lib
fi

#ollama
if [ "$machine" = "PC" ]; then
  sudo snap install curl
  cd ~/RobotNurseHelper_build/
  #This command seems unnecenssary is from https://ollama.com/docs/installation
  curl.snap-acked        #ollama changed its installation script. There is a text explanation in the script. It only accepts Snap-curl and we need to use this command first to prevent a warning message
  curl -fsSL https://ollama.com/install.sh | sh
elif [ "$machine" = "AGXOrin" ]; then
  #There snap program on AGX Orin's Ubuntu is restricted, so we need to use wget
  wget -O- https://ollama.com/install.sh | sh
fi

ollama pull gemma3:1b
if [ "$VRAMSize" -ge 24 ]; then
  ollama pull gemma3:12b
elif [ "$VRAMSize" -ge 16 ]; then
  ollama pull gemma3:4b
fi


#ollama-hpp
cd ~/RobotNurseHelper_build
git clone https://github.com/jmont-dev/ollama-hpp.git
cd ~/RobotNurseHelper_build/ollama-hpp
git checkout v0.9.5
#The ollama.hpp vesion 0.9.7 has a conclict with GCC 13 std::hash (GCC is the default compiler package on Ubuntu 24.04).
#The old verion 0.9.5 does not have this problem because it does not use std::hash. So I use the old version 0.9.5 to prevent the compilation error.
#However, the 0.9.5 version is incompatible with the latest version of cpp-httplib 0.30.0.
#Apply patch to fix compilation with newer cpp-httplib (using sed to avoid patch whitespace issues)
sed -i 's|this->cli->Post("/api/generate", request_string, "application/json", stream_callback)|this->cli->Post("/api/generate", request_string.size(), [\&](size_t offset, size_t length, httplib::DataSink \&sink) { size_t chunk_len = length; size_t remaining = request_string.size() - offset; if (chunk_len > remaining) chunk_len = remaining; sink.write(request_string.data() + offset, chunk_len); return true; }, "application/json", stream_callback, [](uint64_t, uint64_t){ return true; })|' include/ollama.hpp
sed -i 's|this->cli->Post("/api/chat", request_string, "application/json", stream_callback)|this->cli->Post("/api/chat", request_string.size(), [\&](size_t offset, size_t length, httplib::DataSink \&sink) { size_t chunk_len = length; size_t remaining = request_string.size() - offset; if (chunk_len > remaining) chunk_len = remaining; sink.write(request_string.data() + offset, chunk_len); return true; }, "application/json", stream_callback, [](uint64_t, uint64_t){ return true; })|' include/ollama.hpp

#dlib library for face recognition
#The precompiled libdlib-dev does not work. It enables the DLIB_NO_GUI_SUPPORT
#sudo apt -y install libdlib-dev       #Ubuntu 24.04 has dlib version 19.24.0-1 available in its repository
cd ~/RobotNurseHelper_build/
#This command will go wrong in the future because new versions will changes its download URL
#curl https://dlib.net/files/dlib-20.0.tar.bz2 --output dlib-20.0.tar.bz2
wget -O dlib-20.0.tar.bz2 https://dlib.net/files/dlib-20.0.tar.bz2
tar -xjvf dlib-20.0.tar.bz2

#InspireFace (The library has not been tested on AGX Orin, but it should work because it is based on OpenCV and ONNX Runtime, which are both tested on AGX Orin.
#I need to test it on AGX Orin later. If there is a problem, I will try to fix it and update the code.
#if [ "$machine" = "PC" ]; then
  cd ~/RobotNurseHelper_build/
  git clone https://github.com/HyperInspire/InspireFace.git
  checkout v1.2.3
  # Must enter this directory
  cd InspireFace
  # Clone the repository and pull submodules
  git clone --recurse-submodules https://github.com/tunmx/inspireface-3rdparty.git 3rdparty

  # Download lightweight resource files for mobile device
  bash command/download_models_general.sh Pikachu
  # Download resource files for mobile device or PC/server
  bash command/download_models_general.sh Megatron
  # Download resource files for RV1109
  bash command/download_models_general.sh Gundam_RV1109
  # Download resource files for RV1106
  bash command/download_models_general.sh Gundam_RV1106
  # Download resource files for RK356X
  bash command/download_models_general.sh Gundam_RK356X
  # Download resource files for RK3588
  bash command/download_models_general.sh Gundam_RK3588
  # Download resource files for NVIDIA-GPU Device(TensorRT)
  bash command/download_models_general.sh Megatron_TRT

  # Download all model files
  bash command/download_models_general.sh

  # Execute the local compilation script
  bash command/build.sh
#fi

#cpp-httplib
cd ~/RobotNurseHelper_build
git clone https://github.com/yhirose/cpp-httplib.git
cd cpp-httplib
git checkout v0.34.0

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
if [ "$machine" = "PC" ] && [[ "$GPUModel" == "3050laptop" || "$GPUModel" == "4070laptop" ]]; then
  sudo cp ~/RobotNurseHelper/Server/nvidia-power-management.conf /etc/modprobe.d/
  sudo update-initramfs -u
fi

#Dowload media files from internet
cd ~/RobotNurseHelper_build
#wget --no-check-certificate 'https://docs.google.com/uc?export=download&id=1n5GuS6kqABCq5hGLOToFV_11V9mmEHZR' -O RobotNurseHelper.zip
#for large files (>100MB), Google Drive will block the download and ask for a confirmation. Press F12 when you use your brower to download the file. In the Network tab, your will see the authentic url.
wget --no-check-certificate 'https://drive.usercontent.google.com/download?id=1zVMH3INErwSuXJ1TF2gfPe2O0HVLu381&export=download&authuser=0&confirm=t&uuid=d4298704-bde8-444d-9ca4-0e36402c1e1f&at=APcXIO3sT6c9dZSljEclJVnWV_OW%3A1770877164167' -O RobotNurseHelper_MediaFiles.zip
unzip RobotNurseHelper_MediaFiles.zip -d RobotNurseHelper_MediaFiles
cp -r RobotNurseHelper_MediaFiles/RobotNurseHelper/* ~/RobotNurseHelper


#for play video and audio in Qt Multimedia
sudo apt install libcanberra-gtk-module libcanberra-gtk3-module   #for system sound support in Qt Multimedia
if [ "$machine" = "AGXOrin" ]; then
  sudo apt install gstreamer1.0-tools gstreamer1.0-nice gstreamer1.0-qt5 gstreamer1.0-plugins-base
  sudo apt install gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly
  sudo apt install gstreamer1.0-libav
fi 

#install the desktop file to let users launch the program by clicking the icon
#copy the icon file to /usr/share/pixmaps, which is a standard directory for storing icons in Linux systems. This allows the system to find and display the icon properly when you launch the program from the application menu.
sudo cp ~/RobotNurseHelper/Server/ZenboNurse.png /usr/share/pixmaps/ZenboNurse.png

cd ~/RobotNurseHelper/Server
mkdir -p ~/.local/share/applications/ && cp RobotNurseHelper.desktop ~/.local/share/applications/

#update desktop database
update-desktop-database ~/.local/share/applications/

#you need to logout and login again to let the desktop file work. After that, you can launch the program by clicking the icon "Robot Nurse Helper" on your desktop or application menu.

#Yolov11-pose needs CuDNN to run its onnx file. It is only need to install in PC. The AGX Orin has its own GPU acceleration library, which is compatible with CuDNN, so we don't need to install CuDNN on AGX Orin.
if [ "$machine" = "PC" ]; then
  cd ~/RobotNurseHelper_build
  # Download Ubuntu 24.04 spcific keyring
  wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb
  # Install keyring
  sudo dpkg -i cuda-keyring_1.1-1_all.deb
  # update package list
  sudo apt-get update
  # install runtime library for YOLOv11-pose
  sudo apt-get install libcudnn9-cuda-12
  sudo ldconfig
  ls -l /usr/lib/x86_64-linux-gnu/libcudnn.so.9
fi

##############################
#install AnythingLLM for RAG
##############################
#The library is required by AnythingLLM, but missing in Ubuntu 22.04 and 24.04.
sudo apt install libfuse2

cd ~/RobotNurseHelper_build
# Download the installer script to wherever you want to run it from
curl -fsSL https://cdn.anythingllm.com/latest/installer.sh -o installer.sh
 
# Make the script executable
chmod +x installer.sh
 
# Run the script
./installer.sh


##############################
#AnythingLLM Instructions
##############################
echo "Launch AnythingLLM, and do the following things:"
echo "1. Create a new workspace named 'cataract'."
echo "2. Upload the CataractRAG.txt to cataract workspace."
echo "3. Set the workspace LLM provider as Ollama, and set the model as gemma3:1b. If your machine has more than 16G VRAM, you can also set the model as gemma3:4b or gemma3:12b to let the RAG function have a better performance. But it will be slower to generate the first token because the model is larger."
echo "4. Set the System Prompt as the prompt in the AnythingLLM_Setting.txt"
echo "5. Go to the TythingLLM setting/Tools/Developer API to create a new API key."
echo "6. Copy the API key and paste it in the Setting.json to use AnythingLLM's RAG function."
