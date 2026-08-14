# Fix InflateException for InputView in Zenbo flavor

The application crashes during startup in the Zenbo flavor because the `main_activity.xml` layout file refers to `tw.edu.cgu.ai.zenbo.InputView`, but the class is actually located in the `tw.edu.cgu.ai` package.

## Proposed Changes

### robot sub-project

#### [MODIFY] [main_activity.xml](file:///home/chihyuan/RobotNurseHelper/Android/robot/src/Zenbo/res/layout/main_activity.xml)
- Correct the package name for the `InputView` custom view from `tw.edu.cgu.ai.zenbo.InputView` to `tw.edu.cgu.ai.InputView`.

## Verification Plan

### Automated Tests
- Run `gradle :robot:assembleZenboDebug` to ensure the project still builds.

### Manual Verification
- Deploy the app to a Zenbo device (or emulator) and verify that it starts without the `InflateException`.
