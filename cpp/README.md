This folder contains the code files for the server side program of ZenboNurseHelper. It provides an Graphic User Interface (GUI) for a user to remotely control the robot's action. The GUI currently looks like the image below and allows a user to send commands to the robot-side's app, which calls Zenbo SDK to execute those commands.

![GUI](GUI.jpg "GUI")

In this project, we utilize Intel OpenVINO's human_pose_estimation_demo in their Open Model Zoo 2024 demos as a tool to guide our Zenbo robot. Our server-side program receives frames transmitted from the robot-side app, estimates human pose landmark coordinates, and reports the results to the robot-side program.

# Install
We create a script file to install our code and all required libraries including
OpenVINO, OpenCV, git, gcc, Open_Model_Zoo, pip, Protocol Buffer, whisper.cpp, Qt, libgflags, and PortAudio.
The easiest way to install our program is to execute the following script.
```sh
cd ~
wget https://raw.githubusercontent.com/yangchihyuan/ZenboNurseHelper/refs/heads/master/cpp/install.sh
chmod +x install.sh
./install.sh
```

It will ask for your sudo password and take 20 to 30 minutes to install libraries
and compile files. When everything is ready, you can use the following command to launch our program.

```sh
./run_server_side_program.sh
```
The program easily crashes because we use several libraries containing bugs. To detect those bugs, use this command
```sh
./run_server_side_program.sh debug
```
which use gdb for debugging.

# Known problems and workarounds
## Qt FreeType crash problem
Our program often crashes in this function FT_Load_Glyph () at /lib/x86_64-linux-gnu/libfreetype.so.6, which is called by QFontEngineFT::loadGlyph(QFontEngineFT::QGlyphSet*, unsigned int, QFixedPoint const&, QFontEngine::GlyphFormat, bool, bool) const () at /home/chihyuan/Qt/6.7.2/gcc_64/lib/libQt6Gui.so.6. According to two blogs [(Link1)](https://stackoverflow.com/questions/40490414/cannot-trace-cause-of-crash-in-qt-program) [(Link2)](https://blog.csdn.net/weixin_41797797/article/details/105861978), it is a Qt bug only occurring on Linux. To avoid this problem, use the command in the terminal window before launching our program.

