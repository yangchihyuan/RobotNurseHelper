package tw.edu.cgu.ai.kebbi;

import static java.lang.Thread.sleep;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.content.Context;

import com.nuwarobotics.service.agent.NuwaRobotAPI;

import java.io.BufferedInputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import RobotCommandProtobuf.RobotCommandOuterClass;

public class SocketManager {
    public String mServerURL;
    public Integer mPortNumber;

    public Socket mSocketSendImages;      //port 8895
    public Socket mSocketReceiveCommand;     //port 8896
    public Socket mSocketSendAudio;       //port 8897
    public Socket mSocketSendMessages;      //port 8898    //Can I merge this socket with mSocketReceiveCommand?

    private HandlerThread threadSendToServer;
    private Handler handlerSendToServer;
    private HandlerThread threadReceiveCommand;
    private Handler handlerReceiveCommand;
    private boolean mbReceiveCommand;

    private HandlerThread threadCheckDiconnection;
    private Handler handlerCheckDiconnection;

    private HandlerThread threadSendAudio;
    public Handler handlerSendAudio;

    byte[] mMessagePool = new byte[8192];
    int effective_length = 0;
    String beginString = "BeginOfADataFrame";
    String endString = "EndOfADataFrame";

    public NuwaRobotAPI mRobotAPI;
    Converter converter;

    public long dancing_status = 0;
    public Activity activity;
    public boolean bAutoReconnection = true;
    private CameraService cameraService;


    public SocketManager(CameraService cameraService) {
        if( cameraService == null)
            Log.d("debug (C)", "cameraServeice == null");
        this.cameraService = cameraService;
    }

    public void startReceiveCommands()
    {
        //Debug information 2025/4/17. I need to complete this runnable bofore post it again. If there are two runnables in a handler, behaviors become unknown.
        handlerReceiveCommand.post(() -> {
            mbReceiveCommand = true;
            while(mbReceiveCommand) {
                if (mSocketReceiveCommand != null && mSocketReceiveCommand.isConnected()) {
                    try {
                        BufferedInputStream dIn = new BufferedInputStream(mSocketReceiveCommand.getInputStream());
                        int length = 4096;
                        byte[] message = new byte[length];
                        int bytesRead = dIn.read(message, 0, length);
                        if (bytesRead != -1) {
                            System.arraycopy(message, 0, mMessagePool, effective_length, bytesRead);
                            effective_length += bytesRead;
                            String string = new String(mMessagePool, 0, effective_length, StandardCharsets.US_ASCII);

                            int iBegin = string.indexOf(beginString);
                            int iEnd = string.indexOf(endString);
                            Log.d("iBegin", Integer.toString((iBegin)));
                            Log.d("iEnd", Integer.toString((iEnd)));
                            if (iBegin != -1 && iEnd != -1) {
                                byte[] slice = Arrays.copyOfRange(mMessagePool, iBegin + beginString.length(), iEnd);
                                int remaining = effective_length - (iEnd + endString.length());
                                if (remaining > 0) {
                                    System.arraycopy(mMessagePool, (iEnd + endString.length()), mMessagePool, 0, remaining);
                                }
                                effective_length = remaining;

                                RobotCommandOuterClass.RobotCommand command = RobotCommandOuterClass.RobotCommand.parseFrom(slice);
                                Log.d("Debug", "Receive a message");
                                if (command.hasPitch()) {
                                    Log.d("Pitch degree", "Pitch degree " + command.getPitch());
                                    float neckspeed = 40f;      //default
                                    if (command.hasHeadspeed()) {
                                        neckspeed = (float) command.getHeadspeed();
                                    }
                                    mRobotAPI.ctlMotor(1, (float) command.getPitch(), neckspeed);
                                }
                                if (command.hasYaw()) {
                                    Log.d("Yaw degree", "Yaw degree " + command.getYaw());
                                    float neckspeed = 40f;          //default
                                    if (command.hasHeadspeed()) {
                                        neckspeed = (float) command.getHeadspeed();
                                    }
                                    mRobotAPI.ctlMotor(2, (float) command.getYaw(), neckspeed);
                                }

                                if (command.hasSpeakSentence()) {
                                    mRobotAPI.startTTS(command.getSpeakSentence());
//                                        mRobotAPI.showFace();
                                }

                                if (command.hasFace()) {
                                    Log.d("Debug", "Receive a face command");
                                    mRobotAPI.showFace();
                                    //mRobotAPI.playFaceAnimation(command.getFace());    //it does not work.
                                    String[] ttsArray = {"TTS_AngerA", "TTS_AngerB", "TTS_Contempt", "TTS_Disgust", "TTS_Fear", "TTS_JoyA", "TTS_JoyB", "TTS_JoyC", "TTS_PeaceA", "TTS_PeaceB", "TTS_PeaceC", "TTS_SadnessA", "TTS_SadnessB", "TTS_Surprise"};
                                    mRobotAPI.playFaceAnimation(ttsArray[command.getFace()]);     //it works
                                }
                                if( command.hasSface())
                                {
                                    mRobotAPI.showFace();
                                    mRobotAPI.playFaceAnimation(command.getSface());
                                }
                                if (command.hasHideface() && command.getHideface()) {
                                    //I need both commands to hide the face and enable my own activity.
                                    mRobotAPI.hideFace();
                                    mRobotAPI.hideWindow(false);
                                }

                                if (command.hasStopmove()) {
                                    /*
                                    mRobotAPI.motion.stopMoving();   //this function does not work.

                                     */
                                }
                                if (command.hasMotion()) {
                                    String[] motionArray = {"666_TA_DictateL", "666_DA_Full", "666_EM_Mad02", "666_BA_Nodhead",
                                            "666_SP_Swim02", "666_PE_RotateA", "666_SP_Karate", "666_RE_Cheer", "666_SP_Climb", "666_DA_Hit",
                                            "666_TA_DictateR", "666_SP_Bowling", "666_SP_Walk", "666_SA_Find", "666_BA_TurnHead", "666_SA_Toothache",
                                            "666_SA_Sick", "666_SA_Shocked", "666_SP_Dumbbell", "666_SA_Discover", "666_RE_Thanks", "666_PE_Changing",
                                            "666_SP_HorizontalBar", "666_WO_Traffic", "666_RE_HiR", "666_RE_HiL", "666_DA_Brushteeth", "666_RE_Encourage",
                                            "666_RE_Request", "666_PE_Brewing", "666_RE_Change", "666_PE_Phubbing", "666_RE_Baoquan", "666_SP_Cheer",
                                            "666_RE_Ask", "666_PE_Triangel", "666_PE_Sorcery", "666_PE_Sneak", "666_PE_Singing", "666_LE_Yoyo", "666_SP_Throw",
                                            "666_SP_RaceWalk", "666_PE_ShakeFart", "666_PE_RotateC", "666_PE_RotateB", "666_EM_Blush", "666_PE_Puff",
                                            "666_PE_PlayCello", "666_PE_Pikachu"};
                                    Log.d("Debug", "Receive an action command");
                                    mRobotAPI.motionPlay(motionArray[command.getMotion()], true);
                                }
                                if( command.hasSmotion())
                                {
                                    mRobotAPI.motionPlay(command.getSmotion(), true);
                                }
                                if (command.hasDancetype() && command.getDancetype() != 0) {
                                    cameraService.doSomethingThatNotifiesActivity(command.getDancetype());
                                    //I need to hide face, otherwise, the face is still on top of the kebbi's activity.
                                    mRobotAPI.hideFace();
                                }
                                else
                                {
                                    dancing_status = 0;         //This is questionable. The app won't notify the server that the dance complete.
                                }

                                if (command.hasTurnspeed())
                                {
                                    //Control Robot wheel turn left or right circularly
                                    //a positive speed turns right, a negavie speed turns left
                                    //range: -50~+50
                                    mRobotAPI.turn(command.getTurnspeed());
                                }
                                else
                                {
                                    mRobotAPI.turn(0.0f);
                                }
                                //float num1 = 10.2f;
                                //mRobotAPI.turn(num1); //command.getTurnspeed());
                                if( command.hasContent())
                                {
                                    final int REQUEST_CODE = 201 ;
                                    Intent intent = new Intent();
                                    ComponentName comp = new ComponentName("com.nuwarobotics.app.nuwaplayer","com.nuwarobotics.app.nuwaplayer.PlayContentEditorActivity");
                                    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                    intent.setAction("com.nuwarobotics.app.nuwaplayer.action.PLAY_MBTX");
                                    intent.setComponent(comp);
                                    intent.putExtra("PlayId", command.getContent());//the file name put in  /sdcard/contenteditor/
                                    activity.startActivityForResult(intent, REQUEST_CODE);
                                }

                                if( command.hasKillapp() && command.getKillapp())
                                {
                                    android.os.Process.killProcess(android.os.Process.myPid());
                                    // this function only kill this activity
                                    //activity.finish();
                                }
                            }
                        } else {
                            //sleep 30 msecs;
                            sleep(30);
                        }
                    } catch (Exception e) {
                        Log.e("Exception", e.getMessage());
                        try {
                            //Is this the reason the robot kill its connection?
                            Log.d("debug (A)", "mSocketReceiveCommand.close()");
                            mSocketReceiveCommand.close();
                        }
                        catch( Exception e2)
                        {
                            Log.d("closing socket fails", "closing socket fails" + e2.getMessage()); //sendto failed: EPIPE (Broken pipe)
                        }
                        finally
                        {
                            mSocketReceiveCommand = null;
                        }
                    }
                }
            }
        });
    }

    public void stopReceiveCommands()
    {
        mbReceiveCommand = false;
    }
    void sendImage(ByteBuffer imageAndData)
    {
        if( mSocketSendImages != null && mSocketSendImages.isConnected()) {
            final boolean post = handlerSendToServer.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            OutputStream os = mSocketSendImages.getOutputStream();
                            os.write(imageAndData.array());

                        } catch (Exception e) {
                            //Debug Information 2025/4/17, the socket don't change mode even if my server is down
                            try {
                                mSocketSendImages.close();
                            }
                            catch( Exception e2)
                            {
                                Log.d("closing socket fails", "closing socket fails" + e2.getMessage()); //sendto failed: EPIPE (Broken pipe)
                            }
                            finally
                            {
                                mSocketSendImages = null;
                            }
                            Log.d("Exception Send to Server fails", e.getMessage()); //sendto failed: EPIPE (Broken pipe)
                        }
                    }
                }
            );
        }
    }

    void sendImage(RobotCommandOuterClass.RobotToServerMessage message)
    {
        sendAMessage(message, mSocketSendImages);
    }


    void sendAudio(byte[] audioData)
    {
        if( mSocketSendAudio != null && mSocketSendAudio.isConnected()) {
            try {
                OutputStream os = mSocketSendAudio.getOutputStream();
                os.write(audioData);
            } catch (Exception e) {
                try {
                    mSocketSendAudio.close();
                }
                catch( Exception e2)
                {
                    Log.d("closing socket fails", "closing socket fails" + e2.getMessage()); //sendto failed: EPIPE (Broken pipe)
                }
                finally
                {
                    Log.d("Exception Send to Server fails", e.getMessage()); //sendto failed: EPIPE (Broken pipe)
                    mSocketSendAudio = null;
                }
            }
        }
    }

    //Main thread will call this function. Thus, I need to create a new thread to execute it
    public void connectSockets()
    {
//        HandlerThread thread = new HandlerThread("Connect Sockets");
//        thread.start();
//        Handler handler = new Handler(thread.getLooper());

//        handler.post(new Runnable() {
        if( handlerCheckDiconnection != null) {
            handlerCheckDiconnection.post(new Runnable() {
                @Override
                public void run() {
                    try {
                        mSocketSendImages = new Socket(mServerURL, mPortNumber);
                        mSocketReceiveCommand = new Socket(mServerURL, mPortNumber + 1);
                        mSocketSendAudio = new Socket(mServerURL, mPortNumber + 2);
                        mSocketSendMessages = new Socket(mServerURL, mPortNumber + 3);
                    } catch (Exception e) {
                        e.printStackTrace();
                        Log.e("new sockets fail", "new sockets fail" + e.getMessage());
                    }
                }
            });
        }
    }

    public void startThreads() {
        threadSendToServer = new HandlerThread("threadSendToServer");
        threadSendToServer.start();
        handlerSendToServer = new Handler(threadSendToServer.getLooper());

        threadReceiveCommand = new HandlerThread(("threadReceiveCommand"));
        threadReceiveCommand.start();
        handlerReceiveCommand = new Handler(threadReceiveCommand.getLooper());

        threadCheckDiconnection = new HandlerThread(("threadCheckDisconnection"));
        threadCheckDiconnection.start();
        handlerCheckDiconnection = new Handler(threadCheckDiconnection.getLooper());

        threadSendAudio = new HandlerThread(("threadSendAudio"));
        threadSendAudio.start();
        handlerSendAudio = new Handler(threadSendAudio.getLooper());
    }

    public void startDisconnectionChecker()
    {
        bAutoReconnection = true;
        handlerCheckDiconnection.post(new Runnable() {
            @Override
            public void run() {
                try {
                    //Debug Info 25/4/22: This sleep is necenssary. Otherwise, this thread will send another set of connection request.
                    //The establishment of socket connection takes time.
                    sleep(3000);
                } catch (Exception e) {
                    Log.e("SocketManager", "sleep fails " + e.getMessage());
                }

                while(bAutoReconnection) {
                    Log.d("autoReconnection", "enableAutoReconnection");
                    if( mSocketSendImages == null)
                        //launch anther thread to connect sockets
                        for( int i=0; i<200; i++) {
                            if( bAutoReconnection == false)   //The user may cancel the connection while the for loop is running.
                                break;
                            connectSockets();
                            //wait at least for 3 second
                            int sleeptime = (int) Math.pow(2.0, i);
                            if( sleeptime < 3000)
                                sleeptime = 3000;
                            Log.e("SocketManager", Integer.toString(i) + " sleep " + Integer.toString(sleeptime) + " ms");

                            try {
                                sleep(sleeptime);
                            } catch (Exception e) {
                                Log.e("SocketManager", "sleep fails " + e.getMessage());
                            }
                            if(mSocketSendImages != null)
                            {
                                break;
                            }
                        }
                    else {
                        try {
                            sleep(500);
                        } catch (Exception e) {
                            Log.e("Exception enableAutoReconnection", e.getMessage());
                        }
                    }
                }
            }
        });
    }

    public void disconnectSockets()
    {
        bAutoReconnection = false;
        try {
            mSocketSendImages.close();
        } catch (Exception e) {
            Log.e("disconnectSockets SendImages", e.getMessage());
        }

        //Debug Info 25/4/23: The socketRecieveCommand may be broken by the server-side program's error. Thus, I need to close the three sockets separately.
        //Other, the process will jump out of the try when running this command and skip the mSocketSendAudio.close()
        try {
            mSocketReceiveCommand.close();
        } catch (Exception e) {
            Log.e("disconnectSockets ReceiveCommand", e.getMessage());
        }

        try {
            mSocketSendAudio.close();
        } catch (Exception e) {
            Log.e("disconnectSockets SendAudio", e.getMessage());
        }

        try {
            mSocketSendMessages.close();
        } catch (Exception e) {
            Log.e("disconnectSockets SendMessages", e.getMessage());
        }
    }

    public void stopThreads() {
        threadSendToServer.quitSafely();
        threadReceiveCommand.quitSafely();
        threadCheckDiconnection.quitSafely();
        threadSendAudio.quitSafely();
        try {
            threadSendToServer.join();
            threadSendToServer = null;
            handlerSendToServer = null;

            threadReceiveCommand.join();
            threadReceiveCommand = null;
            handlerReceiveCommand = null;

            threadCheckDiconnection.join();
            threadCheckDiconnection = null;
            handlerCheckDiconnection = null;

            threadSendAudio.join();
            threadSendAudio = null;
            handlerSendAudio = null;
        } catch (final InterruptedException e) {
            Log.e("Exception stopThreads", e.getMessage());
        }
    }

    //This is used by 8898. 8895 does not use it.
    public void sendAMessage( RobotCommandOuterClass.RobotToServerMessage message)
    {
        sendAMessage(message, mSocketSendMessages);
    }

    //This function is used by sendAMessage and sendAImage. The only difference is the Socket parameter
    public void sendAMessage( RobotCommandOuterClass.RobotToServerMessage message, Socket mSocket)
    {
        //HandlerThread thread = new HandlerThread("SocketProcess");
        //thread.start();
        //Handler handler = new Handler(thread.getLooper());
        //handler.post(new Runnable() {
        if( handlerSendToServer != null){
            handlerSendToServer.post(new Runnable() {
                @Override
                public void run() {
                    try {
                        if (mSocket.isConnected()) {
                            OutputStream os = mSocket.getOutputStream();
                            os.write("BeginOfADataFrame".getBytes());

                            byte[] byteArray = message.toByteArray();

                            int message_length = byteArray.length;
                            ByteBuffer message_length_buffer = ByteBuffer.allocate(4);
                            message_length_buffer.order(ByteOrder.LITTLE_ENDIAN); // Ubuntu byte order
                            message_length_buffer.putInt(message_length);
                            os.write(message_length_buffer.array());

                            os.write(message.toByteArray());
                            Log.d("Debug", "message length: " + message.getSerializedSize());
                            os.write("EndOfADataFrame".getBytes());
                            }
                    } catch (Exception e) {
                     e.printStackTrace();
                    }
                }
            });
        }
    }

}
