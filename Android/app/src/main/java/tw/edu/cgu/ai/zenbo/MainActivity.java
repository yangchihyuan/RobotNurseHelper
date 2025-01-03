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

import static java.lang.Thread.sleep;

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
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.support.annotation.NonNull;
import android.util.Size;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.asus.robotframework.API.ExpressionConfig;
import com.asus.robotframework.API.MotionControl;
import com.asus.robotframework.API.RobotAPI;
import com.asus.robotframework.API.RobotFace;
import com.asus.robotframework.API.SpeakConfig;
import com.asus.robotframework.API.Utility.PlayAction;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collection;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Queue;
import java.util.TimerTask;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;
import android.widget.Button;

import tw.edu.cgu.ai.zenbo.env.Logger;
import ZenboNurseHelperProtobuf.ServerSend.ReportAndCommand;
import ZenboNurseHelperProtobuf.ServerSend.ReportAndCommand.MoveModeEnum;


public class MainActivity extends Activity {
    private static final int PERMISSIONS_REQUEST = 1;

    private static final String PERMISSION_CAMERA = Manifest.permission.CAMERA;
    private static final String PERMISSION_STORAGE = Manifest.permission.WRITE_EXTERNAL_STORAGE;
    private static final String PERMISSION_RECORD_AUDIO = Manifest.permission.RECORD_AUDIO;

    public RobotAPI mRobotAPI;
    private ZenboCallback robotCallback;
    private static final Logger LOGGER = new Logger();

    private KeyPointView keypointView;
    private InputView inputView;     //2024/6/25 Chih-Yuan Yang: The purpose of the inputView is to get a frame from camera's preview.
    //Thus, I can send the frame to a server.
    private ActionRunnable mActionRunnable = new ActionRunnable();
    private CheckBox checkBox_keep_alert;
    private CheckBox checkBox_enable_connection;
    private CheckBox checkBox_show_face;
    private CheckBox checkBox_dont_move;
    private CheckBox checkBox_dont_rotate;
    private Button button_close;
    private MessageView mMessageView_Detection;
    private MessageView mMessageView_Timestamp;
    private EditText editText_Server;
    private EditText editText_Port;
    private DataBuffer m_DataBuffer;
    private MediaRecorder mMediaRecorder;
    private String mVideoAbsolutePath;
    private Size mPreviewSize = new Size(640, 480);
    private CameraDevice mCameraDevice;
    private HandlerThread threadImageListener;
    private Handler handlerImageListener;
    private HandlerThread threadSendToServer;
    private Handler handlerSendToServer;
    private HandlerThread mThreadAction;    //This thread is used by ActionRunable
    private Handler mHandlerAction;
    private HandlerThread mThreadSendAudio;
    private Handler mHandlerSendAudio;
    private HandlerThread mThreadReceiveCommand;
    private Handler mHandlerReceiveCommand;
    private boolean mbReceiveCommand = true;    //Do I need this variable? Can I delete the thread directly?
    private HandlerThread mThreadExecuteCommand;
    private Handler mHandlerExecuteCommand;



    private ImageReader mPreviewReader;     //used to get onImageAvailable, not used in Camera2Video project
    private CaptureRequest.Builder mPreviewBuilder;
    private final Semaphore cameraOpenCloseLock = new Semaphore(1);
    private boolean mIsRecordingVideo = false;

    private AutoFitTextureView mTextureView;
    private CameraCaptureSession mPreviewSession;
    /*Chih-Yuan Yang 2024/6/16: ImageListener is defined in the ImageListener.java, which is derived
     from ImageReader.OnImageAvailableListener, an interface.
     */
    private final ImageListener mPreviewListener = new ImageListener();
    private final SimpleDateFormat mDateFormat = new SimpleDateFormat("yyyy.MM.dd HH:mm:ss.SSS");
    private static final String[] VIDEO_PERMISSIONS = {
            Manifest.permission.CAMERA,
            Manifest.permission.RECORD_AUDIO
    };
    Socket mSocketReceiveResults;
    Socket mSocketSendImages;
    Socket mSocketSendAudio;
    private String mServerURL;
    private Integer mPortNumber;

    java.util.Timer timer_get_analyzed_results;
    byte[] mMessagePool = new byte[8192];
    int effective_length = 0;
    String beginString = new String("BeginOfAMessage");
    String endString = new String("EndOfAMessage");

    boolean bMoveMode_LookForPeople = true;

    ArrayList<ReportAndCommand> ArrayListCommand = new ArrayList<ReportAndCommand>();

    //TODO: create a queue to store commands
/*    TimerTask task_get_analyzed_results = new TimerTask() {
        public void run() {
        }
    };
*/
    private RobotFace FaceIndexToRobotFace(int FaceIndex)
    {
        RobotFace newFace = RobotFace.DEFAULT;
        switch(FaceIndex) {
            case 0:
                newFace = RobotFace.ACTIVE;
                break;
            case 1:
                newFace = RobotFace.AWARE_LEFT;
                break;
            case 2:
                newFace = RobotFace.AWARE_RIGHT;
                break;
            case 3:
                newFace = RobotFace.CONFIDENT;
                break;
            case 4:
                newFace = RobotFace.DEFAULT;
                break;
            case 5:
                newFace = RobotFace.DEFAULT_STILL;
                break;
            case 6:
                newFace = RobotFace.DOUBTING;
                break;
            case 7:
                newFace = RobotFace.EXPECTING;
                break;
            case 8:
                newFace = RobotFace.HAPPY;
                break;
            case 9:
                newFace = RobotFace.HELPLESS;
                break;
            case 10:
                newFace = RobotFace.HIDEFACE;
                break;
            case 11:
                newFace = RobotFace.IMPATIENT;
                break;
            case 12:
                newFace = RobotFace.INNOCENT;
                break;
            case 13:
                newFace = RobotFace.INTERESTED;
                break;
            case 14:
                newFace = RobotFace.LAZY;
                break;
            case 15:
                newFace = RobotFace.PLEASED;
                break;
            case 16:
                newFace = RobotFace.PRETENDING;
                break;
            case 17:
                newFace = RobotFace.PROUD;
                break;
            case 18:
                newFace = RobotFace.QUESTIONING;
                break;
            case 19:
                newFace = RobotFace.SERIOUS;
                break;
            case 20:
                newFace = RobotFace.SHOCKED;
                break;
            case 21:
                newFace = RobotFace.SHY;
                break;
            case 22:
                newFace = RobotFace.SINGING;
                break;
            case 23:
                newFace = RobotFace.TIRED;
                break;
            case 24:
                newFace = RobotFace.WORRIED;
                break;
        }
        return newFace;
    };

    private int PredefinedActionIndexToPlayAction(int PredefinedActionIndex)
    {

        int theAction = PlayAction.Default_1;
        switch(PredefinedActionIndex) {
            case 0:
                theAction = PlayAction.Body_twist_1;
                break;
            case 1:
                theAction = PlayAction.Body_twist_2;
                break;
            case 2:
                theAction = PlayAction.Dance_2_loop;
                break;
            case 3:
                theAction = PlayAction.Dance_3_loop;
                break;
            case 4:
                theAction = PlayAction.Dance_b_1_loop;
                break;
            case 5:
                theAction = PlayAction.Dance_s_1_loop;
                break;
            case 6:
                theAction = PlayAction.Default_1;
                break;
            case 7:
                theAction = PlayAction.Default_2;
                break;
            case 8:
                theAction = PlayAction.Find_face;
                break;
            case 9:
                theAction = PlayAction.Head_down_1;
                break;
            case 10:
                theAction = PlayAction.Head_down_2;
                break;
            case 11:
                theAction = PlayAction.Head_down_3;
                break;
            case 12:
                theAction = PlayAction.Head_down_4;
                break;
            case 13:
                theAction = PlayAction.Head_down_5;
                break;
            case 14:
                theAction = PlayAction.Head_down_7;
                break;
            case 15:
                theAction = PlayAction.Head_twist_1_loop;
                break;
            case 16:
                theAction = PlayAction.Head_up_1;
                break;
            case 17:
                theAction = PlayAction.Head_up_2;
                break;
            case 18:
                theAction = PlayAction.Head_up_3;
                break;
            case 19:
                theAction = PlayAction.Head_up_4;
                break;
            case 20:
                theAction = PlayAction.Head_up_5;
                break;
            case 21:
                theAction = PlayAction.Head_up_6;
                break;
            case 22:
                theAction = PlayAction.Head_up_7;
                break;
            case 23:
                theAction = PlayAction.Music_1_loop;
                break;
            case 24:
                theAction = PlayAction.Nod_1;
                break;
            case 25:
                theAction = PlayAction.Shake_head_1;
                break;
            case 26:
                theAction = PlayAction.Shake_head_2;
                break;
            case 27:
                theAction = PlayAction.Shake_head_3;
                break;
            case 28:
                theAction = PlayAction.Shake_head_4_loop;
                break;
            case 29:
                theAction = PlayAction.Shake_head_5;
                break;
            case 30:
                theAction = PlayAction.Shake_head_6;
                break;
            case 32:
                theAction = PlayAction.Turn_left_1;
                break;
            case 33:
                theAction = PlayAction.Turn_left_2;
                break;
            case 34:
                theAction = PlayAction.Turn_left_reverse_1;
                break;
            case 35:
                theAction = PlayAction.Turn_left_reverse_2;
                break;
            case 36:
                theAction = PlayAction.Turn_right_1;
                break;
            case 37:
                theAction = PlayAction.Turn_right_2;
                break;
            case 38:
                theAction = PlayAction.Turn_right_reverse_1;
                break;
            case 39:
                theAction = PlayAction.Turn_right_reverse_2;
                break;
        }
        return theAction;
    }


    //2024/6/25 Chih-Yuan Yang: Why do I need the surfaceTextureListener?
    /**
     * {@link android.view.TextureView.SurfaceTextureListener} handles several lifecycle events on a
     * {@link TextureView}.
     */
    private final TextureView.SurfaceTextureListener surfaceTextureListener =
            new TextureView.SurfaceTextureListener() {
                @Override
                public void onSurfaceTextureAvailable(
                        final SurfaceTexture texture, final int width, final int height) {
                    //2025/1/3: Here is a problem. The users may not grant the permission yet.
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
                    boolean bRecordVideo = false;
                    if(bRecordVideo)
                        //Chih-Yuan Yang 2024/6/16: This is the reason I never record videos. I set the boolean variable fixed.
                        startRecordingVideo();      //The startRecordingVideo is called in a callback function. Is it fine?
                    else
                        startPreview();
                    cameraOpenCloseLock.release();  //Chih-Yuan Yang 2024/6/16: The cameraOpenCloseLock is a semaphore.
                }

                @Override
                public void onDisconnected(final CameraDevice cd) {
                    cameraOpenCloseLock.release();
                    cd.close();
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
        mActionRunnable.ZenboAPI = mRobotAPI;
        mActionRunnable.robotCallback = robotCallback;     //I have several variables in the ZenboCallback class, so I need to pass the object to the ActionRunnable object

        mTextureView = (AutoFitTextureView) findViewById(R.id.texture);
        inputView = (InputView) findViewById(R.id.inputview);
        keypointView = (KeyPointView) findViewById(R.id.keypoint);
        editText_Port = (EditText) findViewById(R.id.editText_Port);
        editText_Server = (EditText) findViewById(R.id.editText_Server);
        checkBox_keep_alert = (CheckBox) findViewById(R.id.checkBox_keepalert);
        checkBox_enable_connection = (CheckBox) findViewById(R.id.checkBox_connect);
        checkBox_show_face = (CheckBox) findViewById(R.id.checkBox_ShowFace);
        checkBox_dont_move = (CheckBox) findViewById(R.id.checkBox_DontMove);
        checkBox_dont_rotate = (CheckBox) findViewById(R.id.checkBox_DontRotate);
        mMessageView_Detection = (MessageView) findViewById(R.id.MessageView_Detection);
        mMessageView_Timestamp = (MessageView) findViewById(R.id.MessageView_Timestamp);
        button_close = (Button) findViewById(R.id.button_close);

        //get the default ServerURL
        mServerURL = editText_Server.getText().toString();
        SharedPreferences sharedPref = getSharedPreferences("ZenboNurseHelper_Preference", Context.MODE_PRIVATE);
        String ServerURL = sharedPref.getString("ServerURL", "");
        if( !ServerURL.isEmpty() ){
            editText_Server.setText(ServerURL);
            mServerURL = ServerURL;
        }


        editText_Server.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {   // 按下完成按钮，这里和上面imeOptions对应
                    mServerURL = v.getText().toString();;
                }
                return false;//返回true，保留软键盘。false，隐藏软键盘
            }
        });


        mPortNumber = Integer.parseInt(editText_Port.getText().toString());
        editText_Port.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {   // 按下完成按钮，这里和上面imeOptions对应
                    mPortNumber = Integer.parseInt(v.getText().toString());
                }
                return false;//返回true，保留软键盘。false，隐藏软键盘
            }
        });

        checkBox_enable_connection.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                HandlerThread thread = new HandlerThread("SocketProcess");
                thread.start();
                Handler handler = new Handler(thread.getLooper());
                if (isChecked) {
                    //Save the IP address to SharedPreferences
                    SharedPreferences sharedPref = getSharedPreferences("ZenboNurseHelper_Preference", Context.MODE_PRIVATE);
                    SharedPreferences.Editor editor = sharedPref.edit();
                    editor.putString("ServerURL", editText_Server.getText().toString());
                    editor.apply();

                    recorder.startRecording();
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                mSocketSendImages = new Socket(mServerURL, mPortNumber);
                                if( mSocketSendImages.isConnected()) {
                                    mPreviewListener.set_socket(mSocketSendImages);
                                }
                                mSocketReceiveResults = new Socket(mServerURL, mPortNumber + 1);
                                if( mSocketReceiveResults.isConnected())
                                {
                                    //Enable the receive thread
                                    mHandlerReceiveCommand.post(new Runnable() {
                                        @Override
                                        public void run() {
                                            while(mbReceiveCommand) {      //turn the boolean off when I need to turn off the thread
//                    if (mSocketReceiveResults != null && mSocketReceiveResults.isConnected()) {
                                                try {
                                                    BufferedInputStream dIn = new BufferedInputStream(mSocketReceiveResults.getInputStream());
                                                    int length = 4096;
                                                    byte[] message = new byte[length];
                                                    int bytesRead = dIn.read(message, 0, length);
                                                    if (bytesRead != -1) {
                                                        System.arraycopy(message, 0, mMessagePool, effective_length, bytesRead);
                                                        effective_length += bytesRead;
                                                        String string = new String(mMessagePool, 0, effective_length, StandardCharsets.US_ASCII);

                                                        int iBegin = string.indexOf(beginString);
                                                        int iEnd = string.indexOf(endString);
                                                        if (iBegin != -1 && iEnd != -1) {
                                                            byte[] slice = Arrays.copyOfRange(mMessagePool, iBegin + beginString.length(), iEnd);
                                                            int remaining = effective_length - (iEnd + endString.length());
                                                            if (remaining > 0) {
                                                                System.arraycopy(mMessagePool, (iEnd + endString.length()), mMessagePool, 0, remaining);
                                                            }
                                                            effective_length = remaining;

                                                            ReportAndCommand report = ReportAndCommand.parseFrom(slice);
                                                            //Do I need a mutex here to protect the ArrayList?
                                                            ArrayListCommand.add(report);       //add to ArrayList
                                                            //Post a Runnable here to execute the command?
                                                            mHandlerExecuteCommand.post(new Runnable() {
                                                                @Override
                                                                public void run() {
                                                                    if( !ArrayListCommand.isEmpty())
                                                                    {
                                                                        ReportAndCommand report = ArrayListCommand.get(0);
                                                                        ArrayListCommand.remove(0);

                                                                        if (report.hasTimeStamp()) {
                                                                            if (bMoveMode_LookForPeople) {
                                                                                //                                    Log.d("report", report.toString());
                                                                                m_DataBuffer.AddNewFrame(report);
                                                                                mHandlerAction.post(mActionRunnable);
                                                                                keypointView.setResults(m_DataBuffer.getLatestFrame());
                                                                            }
                                                                        }
                                                                        if (report.hasX()) {
                                                                            Log.d("move body", report.toString());
                                                                            int serial = mRobotAPI.motion.moveBody(((float) report.getX()) / 100.0f, ((float) report.getY()) / 100.0f, report.getDegree());
                                                                            //The serial number will appear in the callback function.
                                                                        }
                                                                        if (report.hasYaw()) {
                                                                            MotionControl.SpeedLevel.Head speed;
                                                                            switch (report.getHeadspeed()) {
                                                                                case 1:
                                                                                    speed = MotionControl.SpeedLevel.Head.L1;
                                                                                    break;
                                                                                case 2:
                                                                                    speed = MotionControl.SpeedLevel.Head.L2;
                                                                                    break;
                                                                                default:
                                                                                    speed = MotionControl.SpeedLevel.Head.L3;
                                                                            }
                                                                            mRobotAPI.motion.moveHead(report.getYaw(), report.getPitch(), speed);
                                                                        }
                                                                        if (report.hasFace() && report.hasSpeakSentence()) {
                                                                            Log.d("report", report.toString());
                                                                            RobotFace newFace = FaceIndexToRobotFace(report.getFace());
                                                                            ExpressionConfig config = new ExpressionConfig();
                                                                            config.volume(report.getVolume()).speed(report.getSpeed()).pitch(report.getSpeakPitch());
                                                                            mRobotAPI.robot.setExpression(newFace, report.getSpeakSentence(), config);
                                                                        } else if (report.hasSpeakSentence()) {
                                                                            Log.d("Speak Sentence", report.getSpeakSentence());
                                                                            SpeakConfig config = new SpeakConfig();
                                                                            config.volume(report.getVolume()).speed(report.getSpeed()).pitch(report.getSpeakPitch());
                                                                            mRobotAPI.robot.speak(report.getSpeakSentence(), config);
                                                                        } else if (report.hasFace()) {
                                                                            RobotFace newFace = FaceIndexToRobotFace(report.getFace());
                                                                            mRobotAPI.robot.setExpression(newFace);
                                                                        }
                                                                        if (report.hasMovemode()) {
                                                                            if (report.getMovemode() == MoveModeEnum.Manual) {
                                                                                bMoveMode_LookForPeople = false;
                                                                            } else if (report.getMovemode() == MoveModeEnum.LookForPeople) {
                                                                                bMoveMode_LookForPeople = true;
                                                                            }
                                                                        }
                                                                        if (report.hasStopmove()) {
                                                                            mRobotAPI.motion.stopMoving();   //this function does not work.
                                                                        }
                                                                        if (report.hasPredefinedAction()) {
                                                                            //it will still return a serial, but for loop action, will the onResult() in the CallBack be called?
                                                                            int serial = mRobotAPI.utility.playAction(PredefinedActionIndexToPlayAction(report.getPredefinedAction()));
                                                                        }
                                                                    }
                                                                }
                                                            });
                                                        }
                                                    }
                                                    else
                                                    {
                                                        //sleep 30 msecs;
                                                        sleep(30);
                                                    }
                                                } catch (Exception e) {
                                                    Log.e("Exception", e.getMessage());
                                                }
                                                //}
                                            }
                                        }
                                    });
                                }
                                mSocketSendAudio = new Socket(mServerURL, mPortNumber + 2);
                            } catch (Exception e) {
                                e.printStackTrace();
                            }

                            if( mSocketSendAudio != null) {
                                //I need to move this piece of code somewhere else

                                status = true;
                                mHandlerSendAudio.post(
                                    new Runnable() {
                                        @Override
                                        public void run() {
                                            try {
                                                short[] buffer = new short[minBufSize];   //minBufSize = 5376
                                                Log.d("VS", "Buffer created of size " + minBufSize);           // every 5 second, the log message occurs. It does not make sense.

                                                OutputStream os = mSocketSendAudio.getOutputStream();
                                                int readSize;
                                                while (status) {
                                                    //reading data from MIC into buffer
                                                    readSize = recorder.read(buffer, 0, buffer.length);
                                                    if( readSize >= 0) {
                                                        byte[] byteBuffer = ShortToByte_Twiddle_Method(buffer);
                                                        os.write(byteBuffer, 0, readSize * 2);
                                                    }
                                                    else
                                                    {
                                                        Log.e("recorder","recorder.read() error");
                                                    }
                                                }
                                            } catch (UnknownHostException e) {
                                                Log.e("VS", "UnknownHostException");
                                            } catch (IOException e) {
                                                e.printStackTrace();
                                                Log.e("VS", "IOException");
                                            }
                                        }
                                    }
                                );
                            }

                        }
                    });
                }
                else {
                    //2025/1/3 the recorder should stop in onPause()
                    recorder.stop();
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            disconnectSockets();
                        }
                    });
                }
            }
        });

        checkBox_show_face.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mActionRunnable.mShowRobotFace = isChecked;
            }
        });

        checkBox_dont_move.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mActionRunnable.bDontMove = isChecked;
            }
        });

        checkBox_dont_rotate.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mActionRunnable.bDontRotateBody = isChecked;
            }
        });

        button_close.setOnClickListener(new View.OnClickListener() {
                                            @Override
                                            public void onClick(View v) {
                                                mActionRunnable.ZenboAPI.release();
                                                finish();
                                                //moveTaskToBack(true);
                                            }
                                        }
        );


        checkBox_keep_alert.setOnCheckedChangeListener( new CheckBox.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                                         boolean isChecked) {
                mActionRunnable.bKeepAlert = isChecked;
            }

        });

        mActionRunnable.setMessageView(mMessageView_Detection, mMessageView_Timestamp);
        m_DataBuffer = new DataBuffer(100);
        mActionRunnable.setDataBuffer(m_DataBuffer);

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

        mRobotAPI.robot.setExpression(RobotFace.HIDEFACE);
        mRobotAPI.robot.setPressOnHeadAction(false);
        mRobotAPI.robot.setVoiceTrigger(false);     //disable the voice trigger

        View decorView = getWindow().getDecorView();
//        int uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
//                | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;       //Why do I still see the navigation bar.
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
        decorView.setSystemUiVisibility(uiOptions);

        if(recorder == null) {
            recorder = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRate, channelConfig, audioFormat, minBufSize * 10);   //5376* 10
            Log.d("VS", "Recorder initialized");
        }

        // When the screen is turned off and turned back on, the SurfaceTexture is already
        // available, and "onSurfaceTextureAvailable" will not be called. In that case, we can open
        // a camera and start preview from here (otherwise, we wait until the surface is ready in
        // the SurfaceTextureListener).
        if (mTextureView.isAvailable()) {
            if( checkSelfPermission(PERMISSION_CAMERA) == PackageManager.PERMISSION_GRANTED)
                openCamera();   //2025/1/3: It is called
        } else {
            mTextureView.setSurfaceTextureListener(surfaceTextureListener);
        }
    }

    @Override
    protected void onPause() {
        try {
            if (mIsRecordingVideo) {
                mMediaRecorder.stop();      //Sometimes I get an error message here, why? Maybe I cannot call the stop() if it is not recording.
                mIsRecordingVideo = false;
            }
        } catch (Exception e) {

        }
        closeCamera();
        stopThreads();
        disconnectSockets();
        status = false;
        if( recorder != null) {
            recorder.release();     //It causes an exception. Why?
            recorder = null;
        }
        Log.d("VS","Recorder released");


        super.onPause();
//        mRobotAPI.robot.setExpression(RobotFace.DEFAULT);
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
    protected void onStop()
    {
        super.onStop();
    }

//    /**
//     * Opens the camera specified by {@link CameraConnectionFragment#cameraId}.
//     */
    @SuppressLint("MissingPermission")
    @TargetApi(Build.VERSION_CODES.M)
    private void openCamera() {
        mTextureView.setAspectRatio(mPreviewSize.getWidth(), mPreviewSize.getHeight());
        final CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            if (!cameraOpenCloseLock.tryAcquire(30000, TimeUnit.MILLISECONDS)) {
                throw new RuntimeException("Time out waiting to lock camera opening.");
            }
            String cameraId = manager.getCameraIdList()[0];    //Chih-Yuan Yang 2024/6/16: Use the first camera, and Zenbo has only 1 camera
            // Choose the sizes for camera preview and video recording
            CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map = characteristics
                    .get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) {
                throw new RuntimeException("Cannot get available preview/video sizes");
            }
            //Chih-Yuan Yang 2024/6/16: The map object is not used anywhere else.

            mMediaRecorder = new MediaRecorder();
            // 4/25/2018 Chih-Yuan: The permission check is done in the TrackActivity.java
            manager.openCamera(cameraId, mStateCallback, handlerImageListener);
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
                mCameraDevice.close();
                mCameraDevice = null;
            }
            if (null != mPreviewReader) {
                mPreviewReader.close();
                mPreviewReader = null;
            }
            if (null != mMediaRecorder) {
                mMediaRecorder.release();
                mMediaRecorder = null;
            }
        } catch (final InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera closing.", e);
        } finally {
            cameraOpenCloseLock.release();
        }
    }

    private void setUpMediaRecorder() throws IOException {
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
        long max_filesize_bytes = 4 * 1024 * 1024 * 1024;  //4Gib
        mMediaRecorder.setMaxFileSize(max_filesize_bytes);      //Does it work?
        mVideoAbsolutePath = getVideoFilePath(this);
        mMediaRecorder.setOutputFile(mVideoAbsolutePath);
        mMediaRecorder.setVideoEncodingBitRate(10000000);
        mMediaRecorder.setVideoFrameRate(30);
        mMediaRecorder.setVideoSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);
        mMediaRecorder.setOnInfoListener(new MediaRecorder.OnInfoListener() {
            @Override
            public void onInfo(MediaRecorder mr, int what, int extra) {
                if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED) {
                    closeCamera();
                    openCamera();
                }
            }
        });
        mMediaRecorder.prepare();
    }

    /**
     * Starts a background thread and its {@link Handler}.
     */
    private void startThreads() {
        threadImageListener = new HandlerThread("ImageListener");
        threadImageListener.start();
        handlerImageListener = new Handler(threadImageListener.getLooper());

        threadSendToServer = new HandlerThread("threadSendToServer");
        threadSendToServer.start();
        handlerSendToServer = new Handler(threadSendToServer.getLooper());

        mThreadAction = new HandlerThread("ActionThread");
        mThreadAction.start();
        mHandlerAction = new Handler(mThreadAction.getLooper());

        mThreadSendAudio = new HandlerThread(("threadSendAudio"));
        mThreadSendAudio.start();
        mHandlerSendAudio = new Handler(mThreadSendAudio.getLooper());

        mThreadReceiveCommand = new HandlerThread(("threadReceiveCommand"));
        mThreadReceiveCommand.start();
        mHandlerReceiveCommand = new Handler(mThreadReceiveCommand.getLooper());

        mThreadExecuteCommand = new HandlerThread(("threadExecuteCommand"));
        mThreadExecuteCommand.start();
        mHandlerExecuteCommand = new Handler(mThreadExecuteCommand.getLooper(), new ExecuteCommandCallback());
    }

    /**
     * Stops the background thread and its {@link Handler}.
     */
    private void stopThreads() {
        threadImageListener.quitSafely();
        threadSendToServer.quitSafely();
        mThreadAction.quitSafely();
        mThreadSendAudio.quitSafely();
        mThreadReceiveCommand.quitSafely();
        mThreadExecuteCommand.quitSafely();

        try {
            threadImageListener.join();
            threadImageListener = null;
            handlerImageListener = null;

            threadSendToServer.join();
            threadSendToServer = null;
            handlerSendToServer = null;

            mThreadAction.join();
            mThreadAction = null;
            mHandlerAction = null;

            mThreadSendAudio.join();
            mThreadSendAudio = null;
            mHandlerSendAudio = null;

            mThreadReceiveCommand.join();
            mThreadReceiveCommand = null;
            mHandlerReceiveCommand = null;

            mThreadExecuteCommand.join();
            mThreadExecuteCommand = null;
            mHandlerExecuteCommand = null;

        } catch (final InterruptedException e) {
            LOGGER.e(e, "Exception!");
        }
    }

    private void disconnectSockets()
    {
        try {
            if( mSocketSendImages != null) {
                mSocketSendImages.close();
                if (mSocketSendImages.isClosed())
                    mSocketSendImages = null;
            }

            if( mSocketReceiveResults != null) {
                mSocketReceiveResults.close();
                if (mSocketReceiveResults.isClosed())
                    mSocketReceiveResults = null;
            }

            if( mSocketSendAudio != null) {
                mSocketSendAudio.close();
                if (mSocketSendAudio.isClosed())
                    mSocketSendAudio = null;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    private String getVideoFilePath(Context context) {
        String path = Environment.getExternalStorageDirectory().toString();
        Date currentTime = Calendar.getInstance().getTime();
        Log.d("getVideoFilePath", mDateFormat.format(currentTime));
        //I have to mkdir the Captures folder.
        File file = new File(path + "/Captures");
        if (!file.exists()) {
            file.mkdirs();
        }
        return path + "/Captures/" + mDateFormat.format(currentTime) + ".mp4";
    }

    //2024/6/25: This function works, and a file will be save in Zenbo's storage. It uses MediaRecorder.
    private void startRecordingVideo() {
        if (null == mCameraDevice || !mTextureView.isAvailable() || null == mPreviewSize) {
            return;
        }
        try {
            closePreviewSession();
            setUpMediaRecorder();
            SurfaceTexture texture = mTextureView.getSurfaceTexture();
            assert texture != null;
            texture.setDefaultBufferSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
            mPreviewBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
            List<Surface> surfaces = new ArrayList<>();

            // Set up Surface for the camera preview
            Surface previewSurface = new Surface(texture);
            surfaces.add(previewSurface);
            mPreviewBuilder.addTarget(previewSurface);

            // Set up Surface for the MediaRecorder
            Surface recorderSurface = mMediaRecorder.getSurface();
            surfaces.add(recorderSurface);
            mPreviewBuilder.addTarget(recorderSurface);

            // Set up Surface for the ImageReader
            //Chih-Yuan Yang 2024/6/16: mPreview Reader is an ImageReader
            mPreviewReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(), ImageFormat.YUV_420_888, 2);
            //Chih-Yuan Yang 2024/6/16: mPreviewListener is an ImageListener
            mPreviewReader.setOnImageAvailableListener(mPreviewListener, handlerImageListener);
            mPreviewBuilder.addTarget(mPreviewReader.getSurface());
            surfaces.add(mPreviewReader.getSurface());

            // Start a capture session
            // Once the session starts, we can update the UI and start recording
            mCameraDevice.createCaptureSession(surfaces, new CameraCaptureSession.StateCallback() {
                @Override
                public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                    mPreviewSession = cameraCaptureSession;
                    updatePreview();
                    mIsRecordingVideo = true;

                    // Start recording
                    //TODO: this statement may cause an exception. Be aware.
                    mMediaRecorder.start();
                }

                @Override
                public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                    showToast("Failed");
//                    Activity activity = getActivity();
//                    if (null != activity) {
//                        Toast.makeText(activity, "Failed", Toast.LENGTH_SHORT).show();
//                    }
                }
            }, handlerImageListener);
        } catch (CameraAccessException | IOException e) {
            e.printStackTrace();
        }
        mPreviewListener.initialize(handlerSendToServer, inputView, mActionRunnable);
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
        if (null == mCameraDevice || !mTextureView.isAvailable() || null == mPreviewSize) {
            return;
        }
        try {
            closePreviewSession();
            //mPreviewBuilder is a CaptureRequest.Builder boject
            mPreviewBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);

            SurfaceTexture texture = mTextureView.getSurfaceTexture();
            assert texture != null;
            final Surface surface = new Surface(texture);   //2024/6/25 Chih-Yuan: Why do I need 2 Surface objects? One for Capture, another for preview.
            texture.setDefaultBufferSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());
            Surface previewSurface = new Surface(texture);
            mPreviewBuilder.addTarget(previewSurface);   //First target: mTextureView, to show on the screen

            // Create the reader for the preview frames.
            // 2024/6/25 Chih-Yuan: mPreviewReader is an ImageReader object, which can read images from a Surface object.
            mPreviewReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(), ImageFormat.YUV_420_888, 2);
            mPreviewReader.setOnImageAvailableListener(mPreviewListener, handlerImageListener);
            mPreviewBuilder.addTarget(mPreviewReader.getSurface());  //Second target: mPreviewReader, to get the Listener

            mCameraDevice.createCaptureSession(
                    Arrays.asList(surface, mPreviewReader.getSurface()),//Collections.singletonList(previewSurface),
                    new CameraCaptureSession.StateCallback() {

                        @Override
                        public void onConfigured(@NonNull CameraCaptureSession session) {
                            mPreviewSession = session;
                            updatePreview();
                        }

                        @Override
                        public void onConfigureFailed(@NonNull CameraCaptureSession session) {
                            showToast("Failed");
//                            Activity activity = getActivity();
//                            if (null != activity) {
//                                Toast.makeText(activity, "Failed", Toast.LENGTH_SHORT).show();
//                            }
                        }
                    }, handlerImageListener);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        mPreviewListener.initialize(handlerSendToServer, inputView, mActionRunnable);
    }

    private byte [] ShortToByte_Twiddle_Method(short [] input)
    {
        int short_index, byte_index;
        int iterations = input.length;

        byte [] buffer = new byte[input.length * 2];

        short_index = byte_index = 0;

        for(/*NOP*/; short_index != iterations; /*NOP*/)
        {
            //if input[short_index] is 10000 = 0x2710. The buffer[0] = 0x10, buffer[1] = 0x27;
            buffer[byte_index]     = (byte) (input[short_index] & 0x00FF);
            buffer[byte_index + 1] = (byte) ((input[short_index] & 0xFF00) >> 8);

            ++short_index; byte_index += 2;
        }

        return buffer;
    }
}
