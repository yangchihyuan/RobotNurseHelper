package tw.edu.cgu.ai.kebbi

import RobotCommandProtobuf.RobotCommandOuterClass
import RobotCommandProtobuf.RobotCommandOuterClass.RobotToServerMessage
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.ComponentName
import android.content.Intent
import android.os.Binder
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.os.Process
import android.util.Log
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
import java.io.BufferedInputStream
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.StandardCharsets
import java.time.Instant
import java.util.Arrays
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class CameraService : LifecycleService() {

    private lateinit var cameraExecutor: ExecutorService
    private var imageAnalysis: ImageAnalysis? = null
    private val notificationId = 1 // Renamed
    private val channelId = "CameraServiceChannel" // Renamed

    var mMessagePool: ByteArray = ByteArray(8192)
    var effective_length: Int = 0
    var beginString: String = "BeginOfADataFrame"
    var endString: String = "EndOfADataFrame"

    var mRobotAPI: NuwaRobotAPI? = null
    private var mRobot: NuwaRobotAPI? = null

    private var socketManager: SocketManager? = null
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

        // init kiwi sdk
        val your_app_package_name = getPackageName()
        val id = IClientId(your_app_package_name)
        mRobot = NuwaRobotAPI(this, id)
        RegisterRobotCallbackFunctions()
        socketManager = SocketManager()
        socketManager?.mRobotAPI = mRobot
        val socketManager = socketManager
        if (socketManager != null) socketManager.startThreads()
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

        cameraProviderFuture.addListener(
            {
                val cameraProvider = cameraProviderFuture.get()

                // Define the ImageAnalysis use case
                imageAnalysis = ImageAnalysis.Builder()
                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                    .setOutputImageFormat(ImageAnalysis.OUTPUT_IMAGE_FORMAT_RGBA_8888)
                    .build()
                    .also {
                        // Moved lambda argument out of parentheses
                        it.setAnalyzer(cameraExecutor) { imageProxy ->
                            // Send to server
                            val bitmap = imageProxyToBitmap(imageProxy)

                            // 2. Convert Bitmap to JPEG byte array
                            val jpegData = bitmapToJpegByteArray(bitmap, 90) // 90 is the compression quality
                            val timestamp_image = System.currentTimeMillis()
                            val Timestamp = timestamp_image.toString()
                            val JPEG_length = String.format("%05d", jpegData.size)
                            val message_length =
                                (Timestamp.length + 1 + 3 + 1 + JPEG_length.length + 1 + jpegData.size) as Int
                            val buffer_length =
                                message_length + 17 + 4 + 15 //"BeginOfADataFrame" and messagelength (4 bytes) and "EndOfADataFrame"
                            val PitchDegree: String?
                            //I no longer need this.
                            PitchDegree = String.format("%03d", 0)
                            val isDancing = String.format("%03d", socketManager?.dancing_status)
                            val buffer = ByteBuffer.allocate(buffer_length)
                            buffer.order(ByteOrder.LITTLE_ENDIAN) // Ubuntu byte order

                            buffer.put("BeginOfADataFrame".toByteArray())
                            buffer.putInt(message_length)
                            buffer.put(Timestamp.toByteArray())
                            buffer.put("_".toByteArray())
                            buffer.put(isDancing.toByteArray())
                            val Null = "\u0000"
                            buffer.put(Null.toByteArray())
                            buffer.put(JPEG_length.toByteArray())
                            buffer.put(Null.toByteArray())
                            buffer.put(jpegData)
                            buffer.put("EndOfADataFrame".toByteArray())

                            socketManager?.sendImage(buffer)

                            // Do something with the JPEG data, e.g., send it over a network
                            Log.d(
                                "JPEGConverter",
                                "JPEG data size: ${jpegData.size} bytes"
                            )                        // This is crucial: close the image to release the buffer
                            imageProxy.close()
                        }
                    }

            },
            ContextCompat.getMainExecutor(this)
        ) // Changed mainExecutor to ContextCompat.getMainExecutor(this)
    }

    override fun onDestroy() {
        super.onDestroy()
        // Shut down the camera executor and unbind camera use cases
        cameraExecutor.shutdown()
        ProcessCameraProvider.getInstance(this).get().unbindAll()
    }

    private var preview: Preview? =
        null        //variable preview can be a androidx.camera.core.Preview or null

    fun startCameraPreview(previewView: PreviewView) {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)
        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()
            val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA

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


    //set mServerURL and PortNumber
    fun SetServerURLandPortNumber(serverURL: String, portNumber: Int) {
        socketManager?.mServerURL = serverURL;
        socketManager?.mPortNumber = portNumber;
        socketManager?.connectSockets();
        socketManager?.startReceiveCommands();
        socketManager?.startDisconnectionChecker();
    }

    fun RegisterRobotCallbackFunctions() {
        mRobot!!.registerRobotEventListener(object : RobotEventListener {
            override fun onWikiServiceStart() {
                Log.d("KEBBI", "onWikiServiceStart")
                //                mRobot.startTTS("Hello, I'm Kebbi.");
            }

            override fun onWikiServiceStop() {
            }

            override fun onWikiServiceCrash() {
            }

            override fun onWikiServiceRecovery() {
            }

            override fun onStartOfMotionPlay(s: String?) {
            }

            override fun onPauseOfMotionPlay(s: String?) {
            }

            override fun onStopOfMotionPlay(s: String?) {
            }

            override fun onCompleteOfMotionPlay(s: String?) {
                Log.d("KEBBI monitor", "onCompleteOfMotionPlay")
                //When a motion completes
                val instant = Instant.now()

                val time = Timestamp.newBuilder()
                    .setSeconds(instant.getEpochSecond())
                    .setNanos(instant.getNano())
                    .build()

                val yaw = mRobot!!.getMotorPresentPositionInDegree(2)
                val pitch = mRobot!!.getMotorPresentPositionInDegree(1)
                val message =
                    RobotToServerMessage.newBuilder()
                        .setDescription("onCompleteOfMotionPlay")
                        .setYaw(yaw)
                        .setPitch(pitch)
                        .setEventTime(time)
                        .build()

                socketManager!!.sendAMessage(message)
            }

            override fun onPlayBackOfMotionPlay(s: String?) {
                //What is this?
                Log.d("KEBBI monitor", "onPlayBackOfMotionPlay")
            }

            override fun onErrorOfMotionPlay(i: Int) {
            }

            override fun onPrepareMotion(b: Boolean, s: String?, v: Float) {
            }

            override fun onCameraOfMotionPlay(s: String?) {
            }

            override fun onGetCameraPose(
                v: Float,
                v1: Float,
                v2: Float,
                v3: Float,
                v4: Float,
                v5: Float,
                v6: Float,
                v7: Float,
                v8: Float,
                v9: Float,
                v10: Float,
                v11: Float
            ) {
            }

            override fun onTouchEvent(i: Int, i1: Int) {
            }

            override fun onPIREvent(i: Int) {
            }

            override fun onTap(i: Int) {
            }

            override fun onLongPress(i: Int) {
            }

            override fun onWindowSurfaceReady() {
            }

            override fun onWindowSurfaceDestroy() {
            }

            override fun onTouchEyes(i: Int, i1: Int) {
            }

            override fun onRawTouch(i: Int, i1: Int, i2: Int) {
            }

            override fun onFaceSpeaker(v: Float) {
            }

            override fun onActionEvent(i: Int, i1: Int) {
            }

            override fun onDropSensorEvent(i: Int) {
            }

            override fun onMotorErrorEvent(i: Int, i1: Int) {
            }
        })


        mRobot!!.registerVoiceEventListener(object : VoiceEventListener {
            override fun onWakeup(b: Boolean, s: String?, v: Float) {
            }

            override fun onTTSComplete(b: Boolean) {
                //the boolean b means isError
                val instant = Instant.now()

                val time = Timestamp.newBuilder()
                    .setSeconds(instant.getEpochSecond())
                    .setNanos(instant.getNano())
                    .build()

                //Test, send a protocol buffer message here
                val message =
                    RobotToServerMessage.newBuilder()
                        .setDescription("onTTSComplete")
                        .setEventTime(time)
                        .build()

                socketManager!!.sendAMessage(message)
            }

            override fun onSpeechRecognizeComplete(
                b: Boolean,
                resultType: VoiceEventListener.ResultType?,
                s: String?
            ) {
            }

            override fun onSpeech2TextComplete(b: Boolean, s: String?) {
            }

            override fun onMixUnderstandComplete(
                b: Boolean,
                resultType: VoiceEventListener.ResultType?,
                s: String?
            ) {
            }

            override fun onSpeechState(
                listenType: VoiceEventListener.ListenType?,
                speechState: SpeechState?
            ) {
                Log.d("SpeechState", "listenType: " + listenType + " speechState: " + speechState)
            }

            override fun onSpeakState(
                speakType: VoiceEventListener.SpeakType?,
                speakState: VoiceEventListener.SpeakState?
            ) {
                //emulator does not call this function
                Log.d("SpeakState", "speakType: " + speakType + " speakState: " + speakState)
                //only START and SPEAKING is called
            }

            override fun onGrammarState(b: Boolean, s: String?) {
            }

            override fun onListenVolumeChanged(listenType: VoiceEventListener.ListenType?, i: Int) {
            }

            override fun onHotwordChange(
                hotwordState: HotwordState?,
                hotwordType: HotwordType?,
                s: String?
            ) {
            }
        })

    }
}