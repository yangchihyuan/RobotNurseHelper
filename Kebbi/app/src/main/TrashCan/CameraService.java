package tw.edu.cgu.ai.kebbi;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.camera.core.CameraSelector;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.Preview;
import androidx.camera.lifecycle.ProcessCameraProvider;
import androidx.camera.view.PreviewView;
import androidx.core.app.NotificationCompat;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.LifecycleService;

import com.google.common.util.concurrent.ListenableFuture; // Added import

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class CameraService extends LifecycleService {

    private ExecutorService cameraExecutor;
    private ImageAnalysis imageAnalysis;
    private final int notificationId = 1;
    private final java.lang.String channelId = "CameraServiceChannel";

    private final IBinder binder = new LocalBinder();

    public class LocalBinder extends Binder {
        public CameraService getService() {
            return CameraService.this;
        }
    }

    @Nullable
    @java.lang.Override
    public IBinder onBind(@NonNull Intent intent) {
        super.onBind(intent);
        return binder;
    }

    @java.lang.Override
    public void onCreate() {
        super.onCreate();
        cameraExecutor = Executors.newSingleThreadExecutor();
        startForegroundService();
    }

    @java.lang.Override
    public int onStartCommand(@Nullable Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        startCamera();
        return START_STICKY;
    }

    private void startForegroundService() {
        createNotificationChannel();
        Notification notification = new NotificationCompat.Builder(this, channelId)
                .setContentTitle("Camera Service")
                .setContentText("Running camera in background")
                .setSmallIcon(R.drawable.ic_camera) // Make sure you have ic_camera in your drawables
                .build();
        startForeground(notificationId, notification);
    }

    private void createNotificationChannel() {
        NotificationChannel serviceChannel = new NotificationChannel(
                channelId,
                "Camera Service Channel",
                NotificationManager.IMPORTANCE_DEFAULT
        );
        NotificationManager manager = getSystemService(NotificationManager.class);
        if (manager != null) {
            manager.createNotificationChannel(serviceChannel);
        }
    }

    private void startCamera() {
        ListenableFuture<ProcessCameraProvider> cameraProviderFuture = ProcessCameraProvider.getInstance(this); // Changed to use imported ListenableFuture

        cameraProviderFuture.addListener(() -> {
            try {
                ProcessCameraProvider cameraProvider = cameraProviderFuture.get();

                imageAnalysis = new ImageAnalysis.Builder()
                        .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                        .build();

                imageAnalysis.setAnalyzer(cameraExecutor, imageProxy -> {
                    // Send to server

                    // This is crucial: close the image to release the buffer
                    imageProxy.close();
                });

                CameraSelector cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA;

                // Unbind any previous use cases
                cameraProvider.unbindAll();

                // Bind the ImageAnalysis use case to the service's lifecycle
                cameraProvider.bindToLifecycle(this, cameraSelector, imageAnalysis);

            } catch (Exception exc) {
                Log.e("CameraService", "Use case binding failed", exc);
            }
        }, ContextCompat.getMainExecutor(this));
    }

    @java.lang.Override
    public void onDestroy() {
        super.onDestroy();
        if (cameraExecutor != null) {
            cameraExecutor.shutdown();
        }
        try {
            ListenableFuture<ProcessCameraProvider> cameraProviderFuture = ProcessCameraProvider.getInstance(this); // Changed to use imported ListenableFuture
            ProcessCameraProvider cameraProvider = cameraProviderFuture.get();
            if (cameraProvider != null) {
                cameraProvider.unbindAll();
            }
        } catch (java.lang.Exception e) {
            Log.e("CameraService", "Error unbinding camera provider", e);
        }
    }

    private Preview preview = null;

    public void startCameraPreview(PreviewView previewView) {
        ListenableFuture<ProcessCameraProvider> cameraProviderFuture = ProcessCameraProvider.getInstance(this); // Changed to use imported ListenableFuture
        cameraProviderFuture.addListener(() -> {
            try {
                ProcessCameraProvider cameraProvider = cameraProviderFuture.get();
                CameraSelector cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA;

                preview = new Preview.Builder().build();
                preview.setSurfaceProvider(previewView.getSurfaceProvider());


                cameraProvider.unbindAll();
                if (imageAnalysis != null) {
                    cameraProvider.bindToLifecycle(this, cameraSelector, preview, imageAnalysis);
                } else {
                    cameraProvider.bindToLifecycle(this, cameraSelector, preview);
                }

            } catch (Exception exc) {
                Log.e("CameraService", "Use case binding failed", exc);
            }
        }, ContextCompat.getMainExecutor(this));
    }
}
