/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*  This file has been modified by Nataniel Ruiz affiliated with Wall Lab
 *  at the Georgia Institute of Technology School of Interactive Computing
 */

package tw.edu.cgu.ai.zenbo;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.ImageReader;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.support.annotation.NonNull;
import android.util.Size;
import android.view.TextureView;
import android.view.View;
import android.view.WindowManager;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Toast;

import com.asus.robotframework.API.RobotAPI;     //How to release this resource in OnDestroy()
import com.asus.robotframework.API.RobotFace;

import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;
import android.widget.Button;

import tw.edu.cgu.ai.zenbo.env.Logger; //Where do I use the Logger?

public class MainActivity extends Activity {
    private static final int PERMISSIONS_REQUEST = 1;

    private static final String PERMISSION_CAMERA = Manifest.permission.CAMERA;
    private static final String PERMISSION_STORAGE = Manifest.permission.WRITE_EXTERNAL_STORAGE;
    private static final String PERMISSION_RECORD_AUDIO = Manifest.permission.RECORD_AUDIO;

    public RobotAPI mRobotAPI;
    private ZenboCallback robotCallback;
    private static final Logger LOGGER = new Logger();

    private InputView inputView;     //2024/6/25 Chih-Yuan Yang: The purpose of the inputView is to get a frame from camera's preview.
    //Thus, I can send the frame to a server.
//    private ActionRunnable mActionRunnable = new ActionRunnable();
    private CheckBox checkBox_enable_connection;
    private EditText editText_Server;
    private EditText editText_Port;
//    private DataBuffer m_DataBuffer;
    //Zenbo supports 1920x1080
    //Emulator only supports up to 1280x960
    private Size mPreviewSize = new Size(640, 480);
    private CameraDevice mCameraDevice;
    private HandlerThread threadImageListener;
    private Handler handlerImageListener;
    private HandlerThread mThreadSendAudio;
    private Handler mHandlerSendAudio;
//    private boolean mbReceiveCommand;

    private ImageReader mPreviewReader;
    private CaptureRequest.Builder mPreviewBuilder;
    private final Semaphore cameraOpenCloseLock = new Semaphore(1);
    private CameraCaptureSession mPreviewSession;
    private final ImageListener mPreviewListener = new ImageListener();
    private final SimpleDateFormat mDateFormat = new SimpleDateFormat("yyyy.MM.dd HH:mm:ss.SSS");

    private SocketManager socketManager = new SocketManager();

    private Converter converter;

    /**
     * {@link android.view.TextureView.SurfaceTextureListener} handles several lifecycle events on a
     * {@link TextureView}.
     */
    private final TextureView.SurfaceTextureListener surfaceTextureListener =
            new TextureView.SurfaceTextureListener() {
                @Override
                public void  onSurfaceTextureAvailable(
                        final SurfaceTexture texture, final int width, final int height) {
                    if( checkSelfPermission(PERMISSION_CAMERA) == PackageManager.PERMISSION_GRANTED )
                        openCamera();
                }

                @Override
                public void onSurfaceTextureSizeChanged(
                        final SurfaceTexture texture, final int width, final int height) {
                }

                @Override
                public boolean onSurfaceTextureDestroyed(final SurfaceTexture texture) {
                    return true;
                }

                @Override
                public void onSurfaceTextureUpdated(final SurfaceTexture texture) {
                }
            };


    /**
     * {@link android.hardware.camera2.CameraDevice.StateCallback}
     * is called when {@link CameraDevice} changes its state.
     */
    private final CameraDevice.StateCallback mStateCallback =
            new CameraDevice.StateCallback() {
                @Override
                public void onOpened(final CameraDevice cameraDevice) {
                    // This method is called when the camera is opened.  We start camera preview here.
                    mCameraDevice = cameraDevice;
                    startPreview();
                    cameraOpenCloseLock.release();  //Chih-Yuan Yang 2024/6/16: The cameraOpenCloseLock is a semaphore.
                }

                @Override
                public void onDisconnected(final CameraDevice cd) {
                    cameraOpenCloseLock.release();
                    cd.close();    //2025/1/6 This function is not called.
                    mCameraDevice = null;
                }

                @Override
                public void onError(final CameraDevice cd, final int error) {
                    cameraOpenCloseLock.release();
                    cd.close();
                    mCameraDevice = null;
                }
            };

    private void showToast(final String text) {
        MainActivity.this.runOnUiThread(
            new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(MainActivity.this, text, Toast.LENGTH_SHORT).show();
                }
            }
        );
    }

    AudioRecord recorder;

    private int sampleRate = 44100 ; // 44100 for music
    private int channelConfig = AudioFormat.CHANNEL_IN_STEREO;
    private int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
    int minBufSize = AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat);   //minBufSize = 5376, but larger is better.
    private boolean status = true;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.main_activity);

        if(!hasPermission()) {
            //This is a new thread, we don't know when users will finish it.
            requestPermission();
        }

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        robotCallback = new ZenboCallback();
        mRobotAPI = new RobotAPI(this, robotCallback);
        socketManager.mRobotAPI = mRobotAPI;
        socketManager.startThreads();

        inputView = (InputView) findViewById(R.id.inputview);
        editText_Port = (EditText) findViewById(R.id.editText_Port);
        editText_Server = (EditText) findViewById(R.id.editText_Server);
        checkBox_enable_connection = (CheckBox) findViewById(R.id.checkBox_connect);
        Button button_close = (Button) findViewById(R.id.button_close);

        //get the default ServerURL
        SharedPreferences sharedPref = getSharedPreferences("ZenboNurseHelper_Preference", Context.MODE_PRIVATE);
        String ServerURL = sharedPref.getString("ServerURL", "");
        if( !ServerURL.isEmpty() ){
            editText_Server.setText(ServerURL);
        }

        checkBox_enable_connection.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    //Save the IP address to SharedPreferences
                    SharedPreferences sharedPref = getSharedPreferences("ZenboNurseHelper_Preference", Context.MODE_PRIVATE);
                    SharedPreferences.Editor editor = sharedPref.edit();
                    editor.putString("ServerURL", editText_Server.getText().toString());
                    editor.apply();

                    recorder.startRecording();    //The recorder means the audio recorder
                    socketManager.mServerURL = editText_Server.getText().toString();
                    socketManager.mPortNumber = Integer.parseInt(editText_Port.getText().toString());
                    Log.d("Record", "Before call connectSockets");
                    socketManager.connectSockets();
                    socketManager.startReceiveCommands();
                    socketManager.startDisconnectionChecker();
                }
                else {
                    //2025/1/3 the recorder should stop in onPause()
//                    mbReceiveCommand = false;   //the app should not mSocketReceiveResults.getInputStream()
                    recorder.stop();
                    socketManager.stopReceiveCommands();
                    socketManager.disconnectSockets();
                }
            }
        });

        button_close.setOnClickListener(new View.OnClickListener() {
                                            @Override
                                            public void onClick(View v) {
                                                finish();
                                            }
                                        }
        );

        mRobotAPI.robot.setExpression(RobotFace.HIDEFACE);
        mRobotAPI.robot.setPressOnHeadAction(false);
        mRobotAPI.robot.setVoiceTrigger(false);     //disable the voice trigger

        mRobotAPI.robot.speak("哈囉，你好。");
    }  //end of onCreate

    //2025/1/3 This is a call back function, using the same thread as onCreate(). Thus, it is only be called after the onCreated is completed.
    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSIONS_REQUEST: {
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED
                        && grantResults[1] == PackageManager.PERMISSION_GRANTED
                        && grantResults[2] == PackageManager.PERMISSION_GRANTED) {
                    startThreads();
                } else {
                    requestPermission();
                }
            }
        }
    }


    @Override
    protected void onResume() {
        super.onResume();
        startThreads();

        View decorView = getWindow().getDecorView();
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
        decorView.setSystemUiVisibility(uiOptions);

        if(recorder == null) {
            recorder = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRate, channelConfig, audioFormat, minBufSize * 10);   //5376* 10
            Log.d("VS", "Recorder initialized");
        }
        if( checkSelfPermission(PERMISSION_CAMERA) == PackageManager.PERMISSION_GRANTED )
            openCamera();

        StartAudioRecorder();
    }

    private void StartAudioRecorder()
    {
        //Start audio recorder
        mHandlerSendAudio.post(
            new Runnable() {
                @Override
                public void run() {
                    short[] buffer = new short[minBufSize];   //minBufSize = 5376
                    Log.d("VS", "Buffer created of size " + minBufSize);           // every 5 second, the log message occurs. It does not make sense.

                    int readSize;
                    while (status) {
                        //reading data from MIC into buffer
                        readSize = recorder.read(buffer, 0, buffer.length);
                        if( readSize >= 0) {
                            byte[] byteBuffer = converter.ShortToByte_Twiddle_Method(buffer);
                            socketManager.sendAudio(byteBuffer);
                        }
                        else
                        {
                            Log.e("recorder","recorder.read() error");
                        }
                    }
                }
            }
        );
    }

    private boolean hasPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return checkSelfPermission(PERMISSION_CAMERA) == PackageManager.PERMISSION_GRANTED &&
                    checkSelfPermission(PERMISSION_STORAGE) == PackageManager.PERMISSION_GRANTED &&
                    checkSelfPermission(PERMISSION_RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED;
        } else {
            return true;
        }
    }

    private void requestPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (shouldShowRequestPermissionRationale(PERMISSION_CAMERA) ||
                    shouldShowRequestPermissionRationale(PERMISSION_STORAGE) ||
                    shouldShowRequestPermissionRationale(PERMISSION_RECORD_AUDIO)) {
                Toast.makeText(this, "Please grant permissions to execute this program", Toast.LENGTH_LONG).show();
            }
            requestPermissions(new String[]{PERMISSION_CAMERA, PERMISSION_STORAGE, PERMISSION_RECORD_AUDIO}, PERMISSIONS_REQUEST);
        }
    }

    @Override
    protected void onPause() {
        closeCamera();
        socketManager.disconnectSockets();
        status = false;
        if( recorder != null) {
            recorder.release();     //It causes an exception. Why?
            recorder = null;
        }
        Log.d("VS","Recorder released");

        super.onPause();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
        stopThreads();      //Which is better? onPause() or onStop()?
        socketManager.stopThreads();
    }

    @Override
    protected void onDestroy()
    {
        if( mRobotAPI != null)
            mRobotAPI.release();
        super.onDestroy();
    }

//    /**
//     * Opens the camera specified by {@link CameraConnectionFragment#cameraId}.
//     */
    @SuppressLint("MissingPermission")
    @TargetApi(Build.VERSION_CODES.M)
    private void openCamera() {
        final CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            if (!cameraOpenCloseLock.tryAcquire(30000, TimeUnit.MILLISECONDS)) {
                throw new RuntimeException("Time out waiting to lock camera opening.");
            }
            String cameraId = manager.getCameraIdList()[0];
            manager.openCamera(cameraId, mStateCallback, handlerImageListener);
        } catch (final CameraAccessException e) {
            LOGGER.e(e, "Exception!");
        } catch (final InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera opening.", e);
        }
    }

    /*
    2025/01/06 Chih-Yuan: This is an utility function. I did not use it for ZenboNurseHelper.
     */
    private void listCameraSupportedFormats() {
        final CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            if (!cameraOpenCloseLock.tryAcquire(30000, TimeUnit.MILLISECONDS)) {
                throw new RuntimeException("Time out waiting to lock camera opening.");
            }
            String cameraId = manager.getCameraIdList()[0];    //Chih-Yuan Yang 2024/6/16: Use the first camera, and Zenbo has only 1 camera
            // Choose the sizes for camera preview and video recording
            CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

            if (map == null) {
                throw new RuntimeException("Cannot get available preview/video sizes");
            }
            else {
                int format_list[] = map.getOutputFormats();
                for( int format : format_list )
                {
                    Log.d("format",String.format("Support format %d", format));
                    Size Sizes[] = map.getOutputSizes(format);
                    for( Size size : Sizes)
                    {
                        Log.d("OutputSizes",String.format("width %d height %d", size.getWidth(), size.getHeight()));
                    }
                }
            }

        } catch (final CameraAccessException e) {
            LOGGER.e(e, "Exception!");
        } catch (final InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera opening.", e);
        }
    }


    /**
     * Closes the current {@link CameraDevice}.
     */
    private void closeCamera() {
        try {
            cameraOpenCloseLock.acquire();
            if (null != mPreviewSession) {
                mPreviewSession.close();
                mPreviewSession = null;
            }
            if (null != mCameraDevice) {
//                mCameraDevice.close();      //2025/1/6 Here is an error message. Do I not need to close the camera again?
                //When does the mCameraDevice object close?
                //the mCameraDevice is close in the backback class's onDisconnected()
                mCameraDevice = null;
            }
            if (null != mPreviewReader) {
                mPreviewReader.close();
                mPreviewReader = null;
            }
        } catch (final InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera closing.", e);
        } finally {
            cameraOpenCloseLock.release();
        }
    }

    /**
     * Starts a background thread and its {@link Handler}.
     */
    private void startThreads() {
        threadImageListener = new HandlerThread("ImageListener");
        threadImageListener.start();
        handlerImageListener = new Handler(threadImageListener.getLooper());

        mThreadSendAudio = new HandlerThread(("threadSendAudio"));
        mThreadSendAudio.start();
        mHandlerSendAudio = new Handler(mThreadSendAudio.getLooper());
    }

    /**
     * Stops the background thread and its {@link Handler}.
     */
    private void stopThreads() {
        threadImageListener.quitSafely();
        mThreadSendAudio.quitSafely();

        try {
            threadImageListener.join();
            threadImageListener = null;
            handlerImageListener = null;

            mThreadSendAudio.join();
            mThreadSendAudio = null;
            mHandlerSendAudio = null;
        } catch (final InterruptedException e) {
            LOGGER.e(e, "Exception!");
        }
    }


    private void closePreviewSession() {
        if (mPreviewSession != null) {
            mPreviewSession.close();
            mPreviewSession = null;
        }
    }

    /**
     * Update the camera preview. {@link #startPreview()} needs to be called in advance.
     */
    private void updatePreview() {
        if (null == mCameraDevice) {
            return;
        }
        try {
            //CaptureRequest.CONTROL_MODE: Overall mode of 3A
            mPreviewBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
            mPreviewSession.setRepeatingRequest(mPreviewBuilder.build(), null, handlerImageListener);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    /**
     * Start the camera preview.
     */
    private void startPreview() {
        if (null == mCameraDevice || null == mPreviewSize) {
            return;
        }
        try {
            closePreviewSession();
            mPreviewBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);

            mPreviewReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(), ImageFormat.YUV_420_888, 2);
//            mPreviewReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(), ImageFormat.JPEG, 2);
            mPreviewReader.setOnImageAvailableListener(mPreviewListener, handlerImageListener);
            mPreviewBuilder.addTarget(mPreviewReader.getSurface());

            mCameraDevice.createCaptureSession(
                   Arrays.asList(mPreviewReader.getSurface()),
                   new CameraCaptureSession.StateCallback() {

                        @Override
                        public void onConfigured(@NonNull CameraCaptureSession session) {
                            mPreviewSession = session;
                            updatePreview();
                        }

                        @Override
                        public void onConfigureFailed(@NonNull CameraCaptureSession session) {
                            showToast("Failed");
                        }
                    }, handlerImageListener);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        mPreviewListener.initialize(socketManager, inputView);
    }

}
