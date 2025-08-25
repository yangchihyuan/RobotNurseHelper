package tw.edu.cgu.ai.kebbi

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Intent
import android.os.IBinder
import android.util.Log
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.app.NotificationCompat
import androidx.lifecycle.LifecycleService
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import android.os.Binder
import androidx.camera.view.PreviewView // Added import for PreviewView
import androidx.core.content.ContextCompat

class CameraService : LifecycleService() {

    private lateinit var cameraExecutor: ExecutorService
    private var imageAnalysis: ImageAnalysis? = null
    private val notificationId = 1 // Renamed
    private val channelId = "CameraServiceChannel" // Renamed

    private val binder = LocalBinder() // Removed semicolon

    inner class LocalBinder : Binder() {
        fun getService(): CameraService = this@CameraService
    }

    override fun onBind(intent: Intent): IBinder {
        super.onBind(intent)
        return binder
    }

    override fun onCreate() {
        super.onCreate()
        cameraExecutor = Executors.newSingleThreadExecutor()
        startForegroundService()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        super.onStartCommand(intent, flags, startId)
        startCamera()
        return START_STICKY
    }

    private fun startForegroundService() {
        createNotificationChannel()
        val notification = NotificationCompat.Builder(this, channelId) // Updated usage
            .setContentTitle("Camera Service")
            .setContentText("Running camera in background")
            .setSmallIcon(R.drawable.ic_camera)
            .build()
        startForeground(notificationId, notification) // Updated usage
    }

    private fun createNotificationChannel() {
        // Removed unnecessary SDK_INT check
        val serviceChannel = NotificationChannel(
            channelId, // Updated usage
            "Camera Service Channel",
            NotificationManager.IMPORTANCE_DEFAULT
        )
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(serviceChannel)
    }

    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()

            // Define the ImageAnalysis use case
            imageAnalysis = ImageAnalysis.Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()
                .also {
                    // Moved lambda argument out of parentheses
                    it.setAnalyzer(cameraExecutor) { imageProxy ->
                        // Your background image processing logic here
                        // For example, you could analyze the image for objects, barcodes, etc.
                        val rotationDegrees = imageProxy.imageInfo.rotationDegrees
                        Log.d("CameraService", "Image analyzed. Rotation: $rotationDegrees")

                        // This is crucial: close the image to release the buffer
                        imageProxy.close()
                    }
                }

            val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA

            try {
                // Unbind any previous use cases
                cameraProvider.unbindAll()

                // Bind the ImageAnalysis use case to the service's lifecycle
                cameraProvider.bindToLifecycle(this, cameraSelector, imageAnalysis)
            } catch (exc: Exception) {
                Log.e("CameraService", "Use case binding failed", exc)
            }

        }, ContextCompat.getMainExecutor(this)) // Changed mainExecutor to ContextCompat.getMainExecutor(this)
    }

    override fun onDestroy() {
        super.onDestroy()
        // Shut down the camera executor and unbind camera use cases
        cameraExecutor.shutdown()
        ProcessCameraProvider.getInstance(this).get().unbindAll()
    }

    // Note: onBind is required for a Service but we can return null for a started service.
//    override fun onBind(intent: Intent): IBinder? {
//        super.onBind(intent)
//        return null
//    }

    // Inside CameraService.kt
// ...
    private var preview: Preview? = null

// In CameraService.kt

    // Change the parameter from SurfaceHolder to PreviewView
    fun startCameraPreview(previewView: PreviewView) { // Changed parameter type to PreviewView
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)
        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()
            val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA

            preview = Preview.Builder().build().also {
                // Corrected line: Use the PreviewView's surfaceProvider property
                it.setSurfaceProvider(previewView.surfaceProvider)
            }

            try {
                cameraProvider.unbindAll()
                cameraProvider.bindToLifecycle(this, cameraSelector, preview, imageAnalysis)
            } catch (exc: Exception) {
                Log.e("CameraService", "Use case binding failed", exc)
            }
        }, ContextCompat.getMainExecutor(this))
    }
}
