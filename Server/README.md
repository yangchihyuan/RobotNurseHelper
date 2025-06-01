This folder contains the code files for the server side program of RobotNurseHelper. It provides an Graphic User Interface (GUI) for a user to remotely control the robot's action. The GUI currently looks like the image below and allows a user to send commands to the robot-side's app, which calls Zenbo SDK or Kebbi SDK to execute those commands.

![GUI](GUI.jpg "GUI")

In this project, we utilize MediaPipe as the pose estimation library to guide our robot. Our server-side program receives frames transmitted from the robot-side app, estimates human pose landmark coordinates, send commands to robot to call robot's movement APIs.

# Install
We create a script file to install our code and all required libraries MediaPipe, OpenCV, git, gcc, Protocol Buffer, Qt, and PortAudio.
The easiest way to install our program is to execute the following script.
```sh
cd ~
wget -O install.sh https://raw.githubusercontent.com/yangchihyuan/RobotNurseHelper/refs/heads/master/Server/install.sh
chmod +x install.sh
./install.sh
```

It will ask for your sudo password and take 40 to 50 minutes to install all required libraries
and compile files. Roughly 50Gib data will be downloaded from the Internet if your Ubuntu 24.04 is just installed without any required libraries. Thus, we recommand run the install script with a high-speed Wi-Fi connection. When everything is ready, you can use the following command to launch our program.

Our server-side program requires a GPU to run Whisper.cpp and Ollama rapidly. If your PC does not have a NVidia GPU, our program still can run, but very slowly, and you can not get response immediately. Because both Whisper.cpp and Ollama have multiple models in different size. We recommand you have 8G VRAM to load the Whisper's ggml-large-v3-turbo model and Ollama's Gemma3:4b model. The minimun accepted VRAM size is 2G. You can load Whisper's small model (VRAM 852mb) and Gemma3:1b model (VRAM 1b).

If your GPU has 8G VRAM, you can use this command
```sh
./run_server_side_program.sh 4070
```
If your GPU has 4G VRAM, you can use this command
```sh
./run_server_side_program.sh 3050
```

# Known problems and workarounds
You cannot install the pre-built OpenCV and Protocol Buffer packages for Ubuntu 24.04. The pre-built OpenCV 4.6.0 conflicts with MediaPipe's dependent OpenCV version in terms of their included Protocol Buffer version.
