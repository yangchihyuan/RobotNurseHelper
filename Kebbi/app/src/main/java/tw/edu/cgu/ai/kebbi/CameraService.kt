package tw.edu.cgu.ai.kebbi

import RobotCommandProtobuf.RobotCommandOuterClass
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
import com.nuwarobotics.service.IClientId
import com.nuwarobotics.service.agent.NuwaRobotAPI
import java.io.BufferedInputStream
import java.net.Socket
import java.nio.charset.StandardCharsets
import java.util.Arrays
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class CameraService : LifecycleService() {

    private lateinit var cameraExecutor: ExecutorService
    private var imageAnalysis: ImageAnalysis? = null
    private val notificationId = 1 // Renamed
    private val channelId = "CameraServiceChannel" // Renamed

    var mServerURL: String? = null
    var mPortNumber: Int = 0

    var mSocketSendImages: Socket? = null //port 8895
    var mSocketReceiveCommand: Socket? = null //port 8896
    var mSocketSendAudio: Socket? = null //port 8897
    var mSocketSendMessages: Socket? =
        null //port 8898    //Can I merge this socket with mSocketReceiveCommand?

    private var threadSendToServer: HandlerThread? = null
    private var handlerSendToServer: Handler? = null
    private var threadReceiveCommand: HandlerThread? = null
    private var handlerReceiveCommand: Handler? = null

    private var mbReceiveCommand = false

    private var threadCheckDiconnection: HandlerThread? = null
    private var handlerCheckDiconnection: Handler? = null

    var mMessagePool: ByteArray = ByteArray(8192)
    var effective_length: Int = 0
    var beginString: String = "BeginOfADataFrame"
    var endString: String = "EndOfADataFrame"

    var mRobotAPI: NuwaRobotAPI? = null
    private var mRobot: NuwaRobotAPI? = null


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
                            val jpegData =
                                bitmapToJpegByteArray(bitmap, 90) // 90 is the compression quality

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

    fun startThreads() {
        threadSendToServer = HandlerThread("threadSendToServer")
        threadSendToServer?.start()
        handlerSendToServer = threadSendToServer?.getLooper()?.let { Handler(it) }

        threadReceiveCommand = HandlerThread(("threadReceiveCommand"))
        threadReceiveCommand?.start()
        handlerReceiveCommand = threadReceiveCommand?.getLooper()?.let { Handler(it) }

        threadCheckDiconnection = HandlerThread(("threadCheckDisconnection"))
        threadCheckDiconnection?.start()
        handlerCheckDiconnection = threadCheckDiconnection?.getLooper()?.let { Handler(it) }
    }

    fun stopThreads() {
        threadSendToServer?.quitSafely()
        threadReceiveCommand?.quitSafely()

        threadCheckDiconnection?.quitSafely()
        try {
            threadSendToServer?.join()
            threadSendToServer = null
            handlerSendToServer = null

            threadReceiveCommand?.join()
            threadReceiveCommand = null
            handlerReceiveCommand = null

            threadCheckDiconnection?.join()
            threadCheckDiconnection = null
            handlerCheckDiconnection = null
        } catch (e: InterruptedException) {
            Log.e("Exception stopThreads", e.message!!)
        }
    }

    fun connectSockets() {
        val thread = HandlerThread("Connect Sockets")
        thread.start()
        val handler = Handler(thread.getLooper())

        handler.post(object : Runnable {
            override fun run() {
                try {
                    mSocketSendImages = Socket(mServerURL, mPortNumber)
                    mSocketReceiveCommand = Socket(mServerURL, mPortNumber + 1)
                    mSocketSendAudio = Socket(mServerURL, mPortNumber + 2)
                    mSocketSendMessages = Socket(mServerURL, mPortNumber + 3)
                } catch (e: java.lang.Exception) {
                    e.printStackTrace()
                    Log.e("new sockets fail", "new sockets fail" + e.message)
                }
            }
        })
    }

    fun startReceiveCommands() {
        //Debug information 2025/4/17. I need to complete this runnable bofore post it again. If there are two runnables in a handler, behaviors become unknown.
        handlerReceiveCommand!!.post(Runnable {
            mbReceiveCommand = true
            //launchPlayer(3);
            //mRobotAPI.hideFace();
            //mRobotAPI.hideWindow(false);
//                mRobotAPI.showFace();                           //2025/8/19 Why do I change face here?
//                mRobotAPI.playFaceAnimation("TTS_PeaceB");
            while (mbReceiveCommand) {
//                    Log.d ("mbReceiveCommand","still running");
                if (mSocketReceiveCommand != null && mSocketReceiveCommand!!.isConnected()) {
//                        Log.d ("mbReceiveCommand","Enter if");
                    try {
                        val dIn = BufferedInputStream(mSocketReceiveCommand!!.getInputStream())
                        //                            Log.d("BufferedInputStream", "created");
                        val length = 4096
                        val message = ByteArray(length)
                        val bytesRead = dIn.read(message, 0, length)
                        //                            Log.d("bytesRead", Integer.toString((bytesRead)));
                        if (bytesRead != -1) {
                            System.arraycopy(message, 0, mMessagePool, effective_length, bytesRead)
                            effective_length += bytesRead
                            val string =
                                String(mMessagePool, 0, effective_length, StandardCharsets.US_ASCII)

                            val iBegin: Int = string.indexOf(beginString)
                            val iEnd: Int = string.indexOf(endString)
                            Log.d("iBegin", (iBegin).toString())
                            Log.d("iEnd", (iEnd).toString())
                            if (iBegin != -1 && iEnd != -1) {
                                val slice: ByteArray = Arrays.copyOfRange(
                                    mMessagePool,
                                    iBegin + beginString.length,
                                    iEnd
                                )
                                val remaining: Int = effective_length - (iEnd + endString.length)
                                if (remaining > 0) {
                                    System.arraycopy(
                                        mMessagePool,
                                        (iEnd + endString.length),
                                        mMessagePool,
                                        0,
                                        remaining
                                    )
                                }
                                effective_length = remaining

                                val command = RobotCommandOuterClass.RobotCommand.parseFrom(slice)
                                Log.d("Debug", "Receive a message")
                                if (command.hasPitch()) {
                                    Log.d("Pitch degree", "Pitch degree " + command.getPitch())
                                    var neckspeed = 40f //default
                                    if (command.hasHeadspeed()) {
                                        neckspeed = command.getHeadspeed().toFloat()
                                    }
                                    mRobotAPI?.ctlMotor(1, command.getPitch().toFloat(), neckspeed)
                                }
                                if (command.hasYaw()) {
                                    Log.d("Yaw degree", "Yaw degree " + command.getYaw())
                                    var neckspeed = 40f //default
                                    if (command.hasHeadspeed()) {
                                        neckspeed = command.getHeadspeed().toFloat()
                                    }
                                    mRobotAPI?.ctlMotor(2, command.getYaw().toFloat(), neckspeed)
                                }

                                if (command.hasSpeakSentence()) {
                                    mRobotAPI?.startTTS(command.getSpeakSentence())
                                    //                                        mRobotAPI.showFace();
                                }

                                if (command.hasFace()) {
                                    Log.d("Debug", "Receive a face command")
                                    mRobotAPI?.showFace()
                                    //mRobotAPI.playFaceAnimation(command.getFace());    //it does not work.
                                    val ttsArray = arrayOf<String?>(
                                        "TTS_AngerA",
                                        "TTS_AngerB",
                                        "TTS_Contempt",
                                        "TTS_Disgust",
                                        "TTS_Fear",
                                        "TTS_JoyA",
                                        "TTS_JoyB",
                                        "TTS_JoyC",
                                        "TTS_PeaceA",
                                        "TTS_PeaceB",
                                        "TTS_PeaceC",
                                        "TTS_SadnessA",
                                        "TTS_SadnessB",
                                        "TTS_Surprise"
                                    )
                                    mRobotAPI?.playFaceAnimation(ttsArray[command.getFace()]) //it works
                                }
                                if (command.hasSface()) {
                                    mRobotAPI?.showFace()
                                    mRobotAPI?.playFaceAnimation(command.getSface())
                                }
                                if (command.hasHideface() && command.getHideface()) {
                                    //I need both commands to hide the face and enable my own activity.
                                    mRobotAPI?.hideFace()
                                    mRobotAPI?.hideWindow(false)
                                }

                                if (command.hasStopmove()) {
                                    /*
                                    mRobotAPI.motion.stopMoving();   //this function does not work.

                                     */
                                }
                                if (command.hasMotion()) {
                                    val motionArray = arrayOf<String?>(
                                        "666_TA_DictateL",
                                        "666_DA_Full",
                                        "666_EM_Mad02",
                                        "666_BA_Nodhead",
                                        "666_SP_Swim02",
                                        "666_PE_RotateA",
                                        "666_SP_Karate",
                                        "666_RE_Cheer",
                                        "666_SP_Climb",
                                        "666_DA_Hit",
                                        "666_TA_DictateR",
                                        "666_SP_Bowling",
                                        "666_SP_Walk",
                                        "666_SA_Find",
                                        "666_BA_TurnHead",
                                        "666_SA_Toothache",
                                        "666_SA_Sick",
                                        "666_SA_Shocked",
                                        "666_SP_Dumbbell",
                                        "666_SA_Discover",
                                        "666_RE_Thanks",
                                        "666_PE_Changing",
                                        "666_SP_HorizontalBar",
                                        "666_WO_Traffic",
                                        "666_RE_HiR",
                                        "666_RE_HiL",
                                        "666_DA_Brushteeth",
                                        "666_RE_Encourage",
                                        "666_RE_Request",
                                        "666_PE_Brewing",
                                        "666_RE_Change",
                                        "666_PE_Phubbing",
                                        "666_RE_Baoquan",
                                        "666_SP_Cheer",
                                        "666_RE_Ask",
                                        "666_PE_Triangel",
                                        "666_PE_Sorcery",
                                        "666_PE_Sneak",
                                        "666_PE_Singing",
                                        "666_LE_Yoyo",
                                        "666_SP_Throw",
                                        "666_SP_RaceWalk",
                                        "666_PE_ShakeFart",
                                        "666_PE_RotateC",
                                        "666_PE_RotateB",
                                        "666_EM_Blush",
                                        "666_PE_Puff",
                                        "666_PE_PlayCello",
                                        "666_PE_Pikachu"
                                    )
                                    Log.d("Debug", "Receive an action command")
                                    mRobotAPI?.motionPlay(motionArray[command.getMotion()], true)
                                }
                                if (command.hasSmotion()) {
                                    mRobotAPI?.motionPlay(command.getSmotion(), true)
                                }
                                if (command.hasDancetype() && command.getDancetype() != 0) {
                                    //launchPlayer(command.getDancetype())        //This is activity's task
                                } else {
                                }

                                if (command.hasTurnspeed()) {
                                    mRobotAPI?.turn(command.getTurnspeed().toFloat())
                                } else {
                                    mRobotAPI?.turn(0.0f)
                                }
                                //float num1 = 10.2f;
                                //mRobotAPI.turn(num1); //command.getTurnspeed());
                                //2025/8/25, this is a piece of experimental code. I am not using it right now.
                                if (command.hasContent()) {
                                    val REQUEST_CODE = 201
                                    val intent = Intent()
                                    val comp = ComponentName(
                                        "com.nuwarobotics.app.nuwaplayer",
                                        "com.nuwarobotics.app.nuwaplayer.PlayContentEditorActivity"
                                    )
                                    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                    intent.setAction("com.nuwarobotics.app.nuwaplayer.action.PLAY_MBTX")
                                    intent.setComponent(comp)
                                    intent.putExtra(
                                        "PlayId",
                                        command.getContent()
                                    ) //the file name put in  /sdcard/contenteditor/
                                    //i should forward this command to the activity in the future
                                    //activity.startActivityForResult(intent, REQUEST_CODE)
                                }

                                if (command.hasKillapp() && command.getKillapp()) {
                                    Process.killProcess(Process.myPid())
                                    // this function only kill this activity
                                    //activity.finish();
                                }
                            }
                        } else {
                            //sleep 30 msecs;
                            Thread.sleep(30)
                        }
                    } catch (e: java.lang.Exception) {
                        Log.e("Exception", e.message!!)
                        try {
                            mSocketReceiveCommand!!.close()
                        } catch (e2: java.lang.Exception) {
                            Log.d(
                                "closing socket fails",
                                "closing socket fails" + e2.message
                            ) //sendto failed: EPIPE (Broken pipe)
                        } finally {
                            mSocketReceiveCommand = null
                        }
                    }
                }
            }
        })
    }

}