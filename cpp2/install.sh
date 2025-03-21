#2025/3/20
#Install Zenbo Nurse Helper cpp2 to Ubuntu 24.04
#Author: Chih-Yuan Yang
#Project: Zenbo Nurse Helper



#Install the compiler
#Your Ubuntu system may be too clear to have the compiler. Please install it by
sudo apt -y install build-essential

#install MediaPipe v0.10.22
git clone https://github.com/google-ai-edge/mediapipe.git
git checkout v0.10.22

#Install Our Files
#Suppose your Open Model Zoo is installed in ~/open_model_zoo. Please git clone this repository into the demos directory.

git clone https://github.com/yangchihyuan/ZenboNurseHelper.git
#copy our code to the mediapipe folder
cp ZenboNurseHelper/cpp2/mediapipe_addition/* mediapipe/mediapipe/

#Qt
#We use it to create our GUI
sudo apt -y install qt6-base-dev    
sudo apt -y install qt6-multimedia-dev
#sudo apt-get -y install qtmultimedia5-dev
#It will install Qt version 6.4.2.

#Hint
#The two commands to install Qt base and multimedia libraries allow you to compile this project. However, they do not isntall Qt Designer, a convenient tool to the GUI file mainwindow.ui. If you want to install Qt Designer, you need to use this command
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
#It will download ggml-base.bin from the HuggingFace website. We need its compiled .o files, which will be used in our server-side program.

make

#Because whisper.cpp runs slowly if it only uses CPUs, we need a GPU to accelerate its computation. In our case, Ubuntu desktop 24.04 installs the NVidia-driver 535 by default. It is not the latest one, but still works.

#Compile and Run
#We need CMake to build open_model_zoo projects.
sudo apt -y install cmake

#Run the OpenVINO's build_demos.sh in ~/open_model_zoo/demos to build this project, and an executable file 9_NurseHelper should be created at ~/omz_demos_build/intel64/Release/ To make it easy, we make s build_demos.sh in the directory ~/open_model_zoo/demos/ZenboNurseHelper/cpp

cd ~/ZenboNurseHelper/cpp2
./build.sh

