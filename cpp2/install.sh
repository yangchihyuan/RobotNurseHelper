#2025/3/20
#Install Zenbo Nurse Helper cpp2 to Ubuntu 24.04
#Author: Chih-Yuan Yang
#Project: Zenbo Nurse Helper



#Install the compiler
#Your Ubuntu system may be too clear to have the compiler. Please install it by
sudo apt -y install build-essential

#install git
sudo apt -y install git

#install zip
sudo apt -y install zip

#install libgtk2.0-dev, which is used in OpenCV to show images
sudo apt -y install libgtk2.0-dev 

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

#intall protobuf 3.19.1
cd ~/Downloads
wget -O protobuf-all-3.19.1.zip https://github.com/protocolbuffers/protobuf/releases/download/v3.19.1/protobuf-all-3.19.1.zip
unzip protobuf-all-3.19.1.zip
cd ~/Downloads/protobuf-3.19.1
./configure
make
make check
sudo make install
sudo ldconfig # refresh shared library cache.


#Install Our Files
#Suppose your Open Model Zoo is installed in ~/open_model_zoo. Please git clone this repository into the demos directory.

cd ~ 
git clone https://github.com/yangchihyuan/ZenboNurseHelper.git
#copy our code to the mediapipe folder
cp -r ~/ZenboNurseHelper/cpp2/mediapipe_addition/* ~/mediapipe/

#Install bazelisk
wget https://github.com/bazelbuild/bazelisk/releases/download/v1.25.0/bazelisk-amd64.deb
sudo dpkg -i bazelisk-amd64.deb

#build libmp library
cd ~/mediapipe
bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 mediapipe/examples/desktop/libmp:libmp.so

#copy the .so file to our folder
mkdir ~/ZenboNurseHelper/cpp2/build
cp ~/mediapipe/bazel-bin/mediapipe/examples/desktop/libmp/libmp.so ~/ZenboNurseHelper/cpp2/build

#Qt
#We use it to create our GUI
sudo apt -y install qt6-base-dev    
sudo apt -y install qt6-multimedia-dev
#sudo apt-get -y install qtmultimedia5-dev
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
make
sudo make install

#On some Linux systems, we need to reload the system to make the library visible.
sudo ldconfig

#whisper.cpp
#It is voice-to-text library and we utilize it on our server-side program to quickly generate sentences spoken by an operator, which will be sent to the Zenbo robot to speak out. There is no package make for the Ubuntu system, and we need to compile it from it source file downloaded from its GitHub repository

cd ~
git clone https://github.com/ggerganov/whisper.cpp.git

#We need a Whisper model. In out program, we use the base model for Mandarin.
cd ~/whisper.cpp
bash ./models/download-ggml-model.sh base
#It will download ggml-base.bin from the HuggingFace website.
make

#Run the OpenVINO's build_demos.sh in ~/open_model_zoo/demos to build this project, and an executable file 9_NurseHelper should be created at ~/omz_demos_build/intel64/Release/ To make it easy, we make s build_demos.sh in the directory ~/open_model_zoo/demos/ZenboNurseHelper/cpp
cd ~/ZenboNurseHelper/cpp2/build
cmake ..
cmake --build . -j $(nproc)
cd ..


