package tw.edu.cgu.ai

import tw.edu.cgu.ai.RobotCommandOuterClass.RobotToServerMessage
import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Intent
import android.graphics.Bitmap
import android.os.Binder
import android.os.Handler
import android.os.IBinder
import android.util.Log
import android.util.Size
import androidx.annotation.RequiresPermission
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleService
import com.google.protobuf.Timestamp
import com.nuwarobotics.service.IClientId
import com.nuwarobotics.service.agent.NuwaRobotAPI
import com.nuwarobotics.service.agent.RobotEventListener
import com.nuwarobotics.service.agent.VoiceEventListener
import com.nuwarobotics.service.agent.VoiceEventListener.HotwordState
import com.nuwarobotics.service.agent.VoiceEventListener.HotwordType
import com.nuwarobotics.service.agent.VoiceEventListener.SpeechState
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.time.Instant
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import androidx.camera.core.resolutionselector.ResolutionStrategy
import androidx.camera.core.resolutionselector.ResolutionSelector
import java.io.ByteArrayOutputStream

class CameraService : LifecycleService() {

    private lateinit var cameraExecutor: ExecutorService
    private var imageAnalysis: ImageAnalysis? = null
    private val notificationId = 1 // Renamed
    private val channelId = "CameraServiceChannel" // Renamed

    private var mRobot: NuwaRobotAPI? = null

    private var socketManager: SocketManager? = null
    private var audioManager: AudioManager? = null
    private val binder = LocalBinder()

    private var callback: ServiceCallback? = null
    interface ServiceCallback {
        fun launchPlayer(dancing : Int?)
    }
    inner class LocalBinder : Binder() {
        fun getService(): CameraService = this@CameraService

        // Method to set the callback
        fun setCallback(callback: ServiceCallback?) {
            this@CameraService.callback = callback
        }
    }

    override fun onBind(intent: Intent): IBinder {
        super.onBind(intent)
        return binder
    }

    fun doSomethingThatNotifiesActivity( dancing: Int?) {
        callback?.launchPlayer(dancing)
    }

    override fun onCreate() {
        super.onCreate()
        cameraExecutor = Executors.newSingleThreadExecutor()
        startForegroundService()

        // init kiwi sdk
        val yourAppPackageName = packageName
        val id = IClientId(yourAppPackageName)
        mRobot = NuwaRobotAPI(this, id)
        registerRobotCallbackFunctions()
        socketManager = SocketManager(this)
        socketManager?.mRobotAPI = mRobot
        socketManager?.startThreads()

        audioManager = AudioManager()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        super.onStartCommand(intent, flags, startId)
        startCamera()
        return START_STICKY
    }

    private fun startForegroundService() {
        createNotificationChannel()
        val notification = NotificationCompat.Builder(this, channelId)
            .setContentTitle("Camera Service")
            .setContentText("Running camera in background")
            .setSmallIcon(R.drawable.ic_camera)
            .build()
        startForeground(notificationId, notification)
    }

    private fun createNotificationChannel() {
        val serviceChannel = NotificationChannel(
            channelId,
            "Camera Service Channel",
            NotificationManager.IMPORTANCE_DEFAULT
        )
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(serviceChannel)
    }

    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener(
            {
                val cameraProvider = cameraProviderFuture.get()

                // Define your preferred size
                //Try larger resolution later to better extract face regions.
                val preferredSize = Size(640, 480) // Example for FHD portrait

                // Create the ResolutionStrategy
                val resolutionStrategy = ResolutionStrategy(
                    preferredSize,
                    ResolutionStrategy.FALLBACK_RULE_CLOSEST_HIGHER_THEN_LOWER
                )

                // Create the ResolutionSelector
                val resolutionSelector = ResolutionSelector.Builder()
                    .setResolutionStrategy(resolutionStrategy)
                    // Optional: You can also set an aspect ratio strategy here if needed
                    // .setResolutionAspectRatios(...)
                    .build()

                // 1. Move the buffer allocation outside the analyzer to reuse it
                // Allocate enough space for your max expected image size + headers
                var reusableBuffer = ByteBuffer.allocate(1024 * 1024).apply {
                    order(ByteOrder.LITTLE_ENDIAN)
                }

                imageAnalysis = ImageAnalysis.Builder()
                    .setResolutionSelector(resolutionSelector)
                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                    .setOutputImageFormat(ImageAnalysis.OUTPUT_IMAGE_FORMAT_RGBA_8888)
                    .build()
                    .also {
                        it.setAnalyzer(cameraExecutor) { imageProxy ->
                            try{
                                // Get timestamp (nanosecond)
                                //val timestampNanos = imageProxy.imageInfo.timestamp   //Monotonic Time (time after system boot)
                                // 1. system clock time (millisecond)
                                val currentTimeMillis = System.currentTimeMillis()

                                // 2. system monotonic time (millisecond)
                                val currentElapsedMillis = android.os.SystemClock.elapsedRealtime()

                                // 3. image monotonic time (nanosecond to millisecond)
                                val imageElapsedMillis = imageProxy.imageInfo.timestamp / 1_000_000

                                // 4. Unix time shift
                                val imageUnixMillis = currentTimeMillis - (currentElapsedMillis - imageElapsedMillis)

                                // 5. convert to Protobuf Timestamp
                                val eventTime = com.google.protobuf.Timestamp.newBuilder()
                                    .setSeconds(imageUnixMillis / 1000)
                                    .setNanos(((imageUnixMillis % 1000) * 1_000_000).toInt())
                                    .build()
                                // Convert to Protobuf's Timestamp (second + nanosecond)
                          //      val seconds = timestampNanos / 1_000_000_000
                           //     val nanos = (timestampNanos % 1_000_000_000).toInt()
                           //     val eventTime = com.google.protobuf.Timestamp.newBuilder()
                           //         .setSeconds(seconds)
                           //         .setNanos(nanos)
                           //         .build()

                                val bitmap = imageProxy.toBitmap()      //Official function
                                val stream = ByteArrayOutputStream()
                                val quality = 90
                                bitmap.compress(Bitmap.CompressFormat.JPEG, quality, stream)
                                val jpegData = stream.toByteArray()
                                val message =
                                    RobotToServerMessage.newBuilder()
                                        .setEventTime(eventTime)
                                        .setJpegdatalength(jpegData.size)
                                        .setJpegdata(com.google.protobuf.ByteString.copyFrom(jpegData))
                                        .build()
                                socketManager?.sendImage(message)
                            } catch (e: Exception) {
                                // Log any errors to prevent crashes and help with debugging
                                Log.e("CameraService", "Error during image analysis: ${e.message}", e)
                            } finally {
                                // This block is crucial. It will always be executed. [3, 5]
                                // This ensures we release the frame back to CameraX. [6]
                                imageProxy.close()
                            }

//                            imageProxy.close()
                        }
                    }

            },
            ContextCompat.getMainExecutor(this)
        )
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraExecutor.shutdown()
        ProcessCameraProvider.getInstance(this).get().unbindAll()
    }

    private var preview: Preview? = null

    fun startCameraPreview(previewView: PreviewView) {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)
        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()
            val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA

            preview = Preview.Builder().build().also {
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

    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    fun startConnection(serverURL: String, portNumber: Int) {
        val currentSocketManager = socketManager
        if (currentSocketManager != null) {
            currentSocketManager.mServerURL = serverURL
            currentSocketManager.mPortNumber = portNumber
            currentSocketManager.connectSockets()
            currentSocketManager.startReceiveCommands()
            currentSocketManager.startDisconnectionChecker()

            audioManager?.startRecording()
            audioManager?.startTransmitToServer(currentSocketManager.handlerSendAudio as Handler, currentSocketManager)
        }
    }

    fun stopConnection() {
        audioManager?.stopRecording()
        socketManager?.stopReceiveCommands()
        socketManager?.disconnectSockets()
    }

    fun registerRobotCallbackFunctions() {
        mRobot?.registerRobotEventListener(object : RobotEventListener {
            override fun onWikiServiceStart() {
                Log.d("KEBBI", "onWikiServiceStart")
            }

            override fun onWikiServiceStop() {}

            override fun onWikiServiceCrash() {}

            override fun onWikiServiceRecovery() {}

            override fun onStartOfMotionPlay(s: String?) {}

            override fun onPauseOfMotionPlay(s: String?) {}

            override fun onStopOfMotionPlay(s: String?) {}

            override fun onCompleteOfMotionPlay(s: String?) {
                Log.d("KEBBI monitor", "onCompleteOfMotionPlay")
                val instant = Instant.now()

                val time = Timestamp.newBuilder()
                    .setSeconds(instant.epochSecond)
                    .setNanos(instant.nano)
                    .build()

                val yaw = mRobot?.getMotorPresentPositionInDegree(2) ?: 0.0f // Provide default if mRobot is null
                val pitch = mRobot?.getMotorPresentPositionInDegree(1) ?: 0.0f // Provide default if mRobot is null
                val message =
                    RobotToServerMessage.newBuilder()
                        .setDescription("onCompleteOfMotionPlay")
                        .setYaw(yaw)
                        .setPitch(pitch)
                        .setEventTime(time)
                        .build()

                socketManager?.sendAMessage(message)
            }

            override fun onPlayBackOfMotionPlay(s: String?) {
                Log.d("KEBBI monitor", "onPlayBackOfMotionPlay")
            }

            override fun onErrorOfMotionPlay(i: Int) {}

            override fun onPrepareMotion(b: Boolean, s: String?, v: Float) {}

            override fun onCameraOfMotionPlay(s: String?) {}

            override fun onGetCameraPose(
                v: Float, v1: Float, v2: Float, v3: Float, v4: Float, v5: Float, v6: Float, v7: Float, v8: Float, v9: Float, v10: Float, v11: Float
            ) {}

            override fun onTouchEvent(i: Int, i1: Int) {}

            override fun onPIREvent(i: Int) {}

            override fun onTap(i: Int) {}

            override fun onLongPress(i: Int) {}

            override fun onWindowSurfaceReady() {}

            override fun onWindowSurfaceDestroy() {}

            override fun onTouchEyes(i: Int, i1: Int) {}

            override fun onRawTouch(i: Int, i1: Int, i2: Int) {}

            override fun onFaceSpeaker(v: Float) {}

            override fun onActionEvent(i: Int, i1: Int) {}

            override fun onDropSensorEvent(i: Int) {}

            override fun onMotorErrorEvent(i: Int, i1: Int) {}
        })

        mRobot?.registerVoiceEventListener(object : VoiceEventListener {
            override fun onWakeup(b: Boolean, s: String?, v: Float) {}

            override fun onTTSComplete(b: Boolean) {
                val instant = Instant.now()

                val time = Timestamp.newBuilder()
                    .setSeconds(instant.epochSecond)
                    .setNanos(instant.nano)
                    .build()

                val message =
                    RobotToServerMessage.newBuilder()
                        .setDescription("onTTSComplete")
                        .setEventTime(time)
                        .build()

                socketManager?.sendAMessage(message)
            }

            override fun onSpeechRecognizeComplete(
                b: Boolean, resultType: VoiceEventListener.ResultType?, s: String?
            ) {}

            override fun onSpeech2TextComplete(b: Boolean, s: String?) {}

            override fun onMixUnderstandComplete(
                b: Boolean, resultType: VoiceEventListener.ResultType?, s: String?
            ) {}

            override fun onSpeechState(
                listenType: VoiceEventListener.ListenType?, speechState: SpeechState?
            ) {
                Log.d("SpeechState", "listenType: $listenType speechState: $speechState")
            }

            override fun onSpeakState(
                speakType: VoiceEventListener.SpeakType?, speakState: VoiceEventListener.SpeakState?
            ) {
                Log.d("SpeakState", "speakType: $speakType speakState: $speakState")
            }

            override fun onGrammarState(b: Boolean, s: String?) {}

            override fun onListenVolumeChanged(listenType: VoiceEventListener.ListenType?, i: Int) {}

            override fun onHotwordChange(
                hotwordState: HotwordState?, hotwordType: HotwordType?, s: String?
            ) {}
        })
    }

    fun NotifyServer(description: String){
        val instant = Instant.now()

        val time = Timestamp.newBuilder()
            .setSeconds(instant.epochSecond)
            .setNanos(instant.nano)
            .build()

        val message =
            RobotToServerMessage.newBuilder()
                .setDescription(description)
                .setEventTime(time)
                .build()

        socketManager?.sendAMessage(message)
    }
}
