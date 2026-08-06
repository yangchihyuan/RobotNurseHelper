package tw.edu.cgu.ai.servicetest

import android.annotation.SuppressLint
import android.app.*
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.ImageFormat
import android.media.Image
import android.os.*
import android.util.Log
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.app.NotificationCompat
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.LifecycleRegistry
import tw.edu.cgu.ai.servicetest.MainActivity
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.Executors

class CameraFrameService : Service(), LifecycleOwner {

    private val TAG = "CameraFrameService"
    private val CHANNEL_ID = "CameraFrameServiceChannel"
    private val NOTIFICATION_ID = 2

    private val lifecycleRegistry: LifecycleRegistry by lazy {
        LifecycleRegistry(this)
    }

    private lateinit var cameraProvider: ProcessCameraProvider
    private val cameraExecutor = Executors.newSingleThreadExecutor()

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    override fun onCreate() {
        super.onCreate()
        lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_CREATE)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        startForeground(NOTIFICATION_ID, createNotification())
        lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_START)

        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)
        cameraProviderFuture.addListener({
            cameraProvider = cameraProviderFuture.get()
            bindCameraUseCases()
            Log.d(TAG, "Camera is ready and bound for analysis.")
        }, cameraExecutor)

        return START_STICKY
    }

    private fun bindCameraUseCases() {
        cameraProvider.unbindAll()

        val cameraSelector = CameraSelector.Builder()
            .requireLensFacing(CameraSelector.LENS_FACING_BACK)
            .build()

        val imageAnalysis = ImageAnalysis.Builder()
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .build()

        // Set the analyzer to receive frames
        imageAnalysis.setAnalyzer(cameraExecutor, FrameAnalyzer())

        try {
            cameraProvider.bindToLifecycle(this, cameraSelector, imageAnalysis)
        } catch (e: Exception) {
            Log.e(TAG, "Use case binding failed", e)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraExecutor.shutdown()
        cameraProvider.unbindAll()
        lifecycleRegistry.handleLifecycleEvent(Lifecycle.Event.ON_DESTROY)
        Log.d(TAG, "CameraFrameService destroyed.")
    }

    // [createNotification() method is the same as the previous example]
    private fun createNotification(): Notification {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                CHANNEL_ID,
                "Camera Frame Service Channel",
                NotificationManager.IMPORTANCE_LOW
            )
            val manager = getSystemService(NotificationManager::class.java)
            manager.createNotificationChannel(serviceChannel)
        }

        val notificationIntent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, notificationIntent, PendingIntent.FLAG_IMMUTABLE
        )

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Camera Frame Analysis")
            .setContentText("Your app is processing camera frames.")
            .setSmallIcon(R.drawable.ic_launcher_foreground) // Replace with your app icon
            .setContentIntent(pendingIntent)
            .build()
    }

    override fun getLifecycle(): Lifecycle {
        return lifecycleRegistry
    }

    // The ImageAnalyzer class that processes frames
    private inner class FrameAnalyzer : ImageAnalysis.Analyzer {

        private var frameCounter = 0
        private val savedFramesDir by lazy {
            val dir = File(externalMediaDirs.firstOrNull(), "CameraFrames")
            if (!dir.exists()) dir.mkdirs()
            dir
        }

        @SuppressLint("UnsafeOptInUsageError")
        override fun analyze(imageProxy: ImageProxy) {
            frameCounter++
            // Process a frame every 10 frames to avoid resource overload
            if (frameCounter % 10 == 0) {
                // The ImageProxy.image is a YUV_420_888 format image
                val image = imageProxy.image
                if (image != null) {
                    saveImageToDisk(image)
                }
            }
            // IMPORTANT: Close the image proxy to release the buffer
            imageProxy.close()
        }

        private fun saveImageToDisk(image: Image) {
            // This is a simplified way to save a YUV_420_888 image to disk as a JPG.
            // Converting YUV to JPEG is computationally expensive, so for real-time
            // processing, you might want to use a more efficient format.
            try {
                val planes = image.planes
                val yBuffer = planes[0].buffer // Y plane
                val uBuffer = planes[1].buffer // U plane
                val vBuffer = planes[2].buffer // V plane

                val ySize = yBuffer.remaining()
                val uSize = uBuffer.remaining()
                val vSize = vBuffer.remaining()

                val nv21 = ByteArray(ySize + uSize + vSize)

                // The following conversion logic is for illustrative purposes.
                // It can be optimized for performance.
                yBuffer.get(nv21, 0, ySize)
                vBuffer.get(nv21, ySize, vSize)
                uBuffer.get(nv21, ySize + vSize, uSize)

                val out = FileOutputStream(File(savedFramesDir, "frame_${System.currentTimeMillis()}.jpg"))
                out.write(nv21)
                out.close()
                Log.d(TAG, "Frame saved to disk.")

            } catch (e: Exception) {
                Log.e(TAG, "Error saving frame: ${e.message}", e)
            } finally {
                image.close()
            }
        }
    }
}