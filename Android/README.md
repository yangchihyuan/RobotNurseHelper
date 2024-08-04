This folder contains the source files of our robot-side app of the ZenboNurseHelper project.

# Requirement
This app runs on a Zenbo robot made by Asus Inc. Although this app can be installed on other Android devices, the Zenbo's features will not work.

# Developing Tool
Android Studio Koala (2024.1.1)

# Platform
Ubuntu 24.04

# Build
Just use Android Studio to open this directory and the gradle will automatically build this project.

# Installation
The installation of this app is the same as ones of other Android apps. You can turn on your Zenbo robot's developer options as well as the USB debugging/installing options. Thereafter, you can install the app from the Android Studio through a USB cable connecting to the Zenbo robot by "Running the app on the Zenbo device".
Another way is to build an APK file, copy the APK file to a USB thumb disk, and install the app through the APK file by inserting the USB thumb disk on a USB port on the Zenbo robot's head.

# Hint
If you use the USB debugging mode to install this app. You need to put your Ubuntu account in the plugdev group. Use this command to check whether your account is in the group already.
```sh
cat /etc/group | grep plugdev
```
If you can find your user name, your account is already in the group.
Otherwise, use this command to add your account into the group
```sh
sudo adduser <your user name> plugdev
```

# Run the app
You can find the app 'Zenbo Nurse Helper' in both the robot's Android apps panel.

# Known problems and workarounds
## libprotobuf incompatible problem
Zenbo's Android version is 6, which is too old to be supported by the newly released protobuf libraries. The app will raise an exception if it receive a message with a variable of the type boolean or float. However, there is no exception for the types string, int32 and int64. Therefore, in our ServerSend.proto file, we replace the type of OpenPoseCoordinate.x and OpenPoseCoordinate.y from float to int32 to shun this problem.
