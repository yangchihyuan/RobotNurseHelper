# Fix UnsatisfiedLinkError for libnative-lib.so

The application is failing with `java.lang.UnsatisfiedLinkError` because `libnative-lib.so` is not found at runtime. This is caused by missing CMake configuration in the `robot/build.gradle` file, which prevents the native library from being built and included in the APK. Additionally, the JNI method names in the C++ code do not match the Java package structure, which would cause further errors once the library is loaded.

## Proposed Changes

### [robot]

#### [MODIFY] [build.gradle](file:///home/chihyuan/RobotNurseHelper/Android/robot/build.gradle)
- Add the `externalNativeBuild` block to the `android` section to specify the path to `CMakeLists.txt`. This will enable the native build process for the module.

#### [MODIFY] [imageutils_jni.cc](file:///home/chihyuan/RobotNurseHelper/Android/robot/src/main/jni/imageutils_jni.cc)
- Update the `IMAGEUTILS_METHOD` macro to match the correct Java package `tw.edu.cgu.ai.env`. The current macro includes an incorrect `.kebbi` segment.

## Verification Plan

### Automated Tests
- Run `./gradlew :robot:assembleZenboDebug` to verify that the project builds successfully and the native library is compiled.
- Sync the project with Gradle in Android Studio to ensure all configurations are correctly recognized.

### Manual Verification
- Deploy the app to a Zenbo device (or emulator with appropriate ABI) and verify that the `UnsatisfiedLinkError` is no longer thrown.
