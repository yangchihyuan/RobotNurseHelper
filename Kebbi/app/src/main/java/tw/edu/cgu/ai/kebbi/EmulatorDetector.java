package tw.edu.cgu.ai.kebbi;

import android.os.Build;
import android.util.Log;

public class EmulatorDetector {

    private static final String TAG = "EmulatorDetector";
    private static Boolean isEmulatorCached = null;

    public static boolean isEmulator() {
        if (isEmulatorCached != null) {
            return isEmulatorCached;
        }

        boolean isEmulator = false;

        String fingerprint = Build.FINGERPRINT;
        isEmulator |= fingerprint.startsWith("generic")
                || fingerprint.contains("vbox")
                || fingerprint.contains("sdk_gphone");

        String product = Build.PRODUCT;
        isEmulator |= product.equals("sdk")
                || product.contains("_sdk")
                || product.equals("google_sdk");

        String hardware = Build.HARDWARE;
        isEmulator |= hardware.equals("goldfish")
                || hardware.equals("ranchu");

        String manufacturer = Build.MANUFACTURER;
        isEmulator |= manufacturer.contains("Genymotion");

        String brand = Build.BRAND;
        isEmulator |= brand.startsWith("generic") && Build.DEVICE.startsWith("generic");

        String model = Build.MODEL;
        isEmulator |= model.contains("google_sdk")
                || model.contains("Emulator")
                || model.contains("Android SDK built for x86");

        isEmulatorCached = isEmulator;
        if (isEmulator) {
            Log.i(TAG, "Application is running on an emulator.");
        } else {
            Log.i(TAG, "Application is running on a real device.");
        }
        return isEmulator;
    }
}

