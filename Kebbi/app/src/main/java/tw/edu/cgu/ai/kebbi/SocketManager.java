package tw.edu.cgu.ai.kebbi;

import static java.lang.Thread.sleep;

import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import com.nuwarobotics.service.agent.NuwaRobotAPI;

import java.io.BufferedInputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;

import RobotCommandProtobuf.RobotCommand;

public class SocketManager {
    public String mServerURL;
    public Integer mPortNumber;

    public Socket socketReceiveCommand;
    public Socket mSocketSendImages;
    public Socket mSocketSendAudio;

    private HandlerThread threadSendToServer;
    private Handler handlerSendToServer;
    private HandlerThread threadReceiveCommand;
    private Handler handlerReceiveCommand;
    private HandlerThread mThreadExecuteCommand;
    private Handler mHandlerExecuteCommand;
    private boolean mbReceiveCommand;

    private HandlerThread threadCheckDiconnection;
    private Handler handlerCheckDiconnection;

    byte[] mMessagePool = new byte[8192];
    int effective_length = 0;
    String beginString = new String("BeginOfAMessage");
    String endString = new String("EndOfAMessage");

    public NuwaRobotAPI mRobotAPI;
    ArrayList<RobotCommand.KebbiCommand> ArrayListCommand = new ArrayList<RobotCommand.KebbiCommand>();
    Converter converter;

    public boolean bAutoReconnection = true;
    public void startReceiveCommands()
    {
        //Debug information 2025/4/17. I need to complete this runnable bofore post it again. If there are two runnables in a handler, behaviors become unknown.
        handlerReceiveCommand.post(new Runnable() {
            @Override
            public void run() {
                mbReceiveCommand = true;
                while(mbReceiveCommand) {
//                    Log.d ("mbReceiveCommand","still running");
                    if (socketReceiveCommand != null && socketReceiveCommand.isConnected()) {
//                        Log.d ("mbReceiveCommand","Enter if");
                        try {
                            BufferedInputStream dIn = new BufferedInputStream(socketReceiveCommand.getInputStream());
//                            Log.d("BufferedInputStream", "created");
                            int length = 4096;
                            byte[] message = new byte[length];
                            int bytesRead = dIn.read(message, 0, length);
//                            Log.d("bytesRead", Integer.toString((bytesRead)));
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

                                    RobotCommand.KebbiCommand command = RobotCommand.KebbiCommand.parseFrom(slice);
                                    Log.d("Debug", "Receive a message");
                                    if (command.hasPitch()) {
                                        Log.d("Pitch degree", "Pitch degree " + Integer.toString( command.getPitch()));
                                        float neckspeed = 40f;      //default
                                        if( command.hasHeadspeed())
                                        {
                                            neckspeed = (float) command.getHeadspeed();
                                        }
                                        mRobotAPI.ctlMotor(1,(float)command.getPitch(), neckspeed);
                                    }
                                    if (command.hasYaw()) {
                                        Log.d("Yaw degree", "Yaw degree " + Integer.toString( command.getYaw()));
                                        float neckspeed = 40f;          //default
                                        if( command.hasHeadspeed())
                                        {
                                            neckspeed = (float) command.getHeadspeed();
                                        }
                                        mRobotAPI.ctlMotor(2, (float) command.getYaw(), neckspeed);
                                    }

                                    if (command.hasSpeakSentence())
                                        mRobotAPI.startTTS(command.getSpeakSentence());

                                    if (command.hasFace()) {
                                        Log.d("Debug", "Receive a face command");
                                        mRobotAPI.showFace();
                                        //mRobotAPI.playFaceAnimation(command.getFace());    //it does not work.
                                        String[] ttsArray = {"TTS_AngerA", "TTS_AngerB", "TTS_Contempt", "TTS_Disgust", "TTS_Fear", "TTS_JoyA", "TTS_JoyB", "TTS_JoyC", "TTS_PeaceA", "TTS_PeaceB", "TTS_PeaceC", "TTS_SadnessA", "TTS_SadnessB", "TTS_Surprise"};
                                        mRobotAPI.playFaceAnimation(ttsArray[command.getFace()]);     //it works
                                    }
                                    if( command.hasHideface() && command.getHideface() == true)
                                    {
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
                                                "666_TA_DictateR", "666_SP_Bowling", "666_SP_Walk", "666_SA_Find", "666_BA_TurnHead", "666_SA_Toothache"};
                                        Log.d("Debug", "Receive an action command");
                                        mRobotAPI.motionPlay(motionArray[command.getMotion()], true);
                                    }
                                }
                            } else {
                                //sleep 30 msecs;
                                sleep(30);
                            }
                        } catch (Exception e) {
                            Log.e("Exception", e.getMessage());
                            try {
                                socketReceiveCommand.close();
                            }
                            catch( Exception e2)
                            {
                                Log.d("closing socket fails", "closing socket fails" + e2.getMessage()); //sendto failed: EPIPE (Broken pipe)
                            }
                            finally
                            {
                                socketReceiveCommand = null;
                            }
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
        HandlerThread thread = new HandlerThread("Connect Sockets");
        thread.start();
        Handler handler = new Handler(thread.getLooper());

        handler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    mSocketSendImages = new Socket(mServerURL, mPortNumber);
                    socketReceiveCommand = new Socket(mServerURL, mPortNumber+1);
                    mSocketSendAudio =  new Socket(mServerURL, mPortNumber+2);
                } catch (Exception e) {
                    e.printStackTrace();
                    Log.e("new sockets fail", "new sockets fail" + e.getMessage());
                }
            }
        });
    }

    public void startThreads() {
        threadSendToServer = new HandlerThread("threadSendToServer");
        threadSendToServer.start();
        handlerSendToServer = new Handler(threadSendToServer.getLooper());

        threadReceiveCommand = new HandlerThread(("threadReceiveCommand"));
        threadReceiveCommand.start();
        handlerReceiveCommand = new Handler(threadReceiveCommand.getLooper());

        mThreadExecuteCommand = new HandlerThread(("threadExecuteCommand"));
        mThreadExecuteCommand.start();
        mHandlerExecuteCommand = new Handler(mThreadExecuteCommand.getLooper(), new ExecuteCommandCallback());

        threadCheckDiconnection = new HandlerThread(("threadCheckDisconnection"));
        threadCheckDiconnection.start();
        handlerCheckDiconnection = new Handler(threadCheckDiconnection.getLooper());
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
            socketReceiveCommand.close();
        } catch (Exception e) {
            Log.e("disconnectSockets ReceiveCommand", e.getMessage());
        }

        try {
            mSocketSendAudio.close();
        } catch (Exception e) {
            Log.e("disconnectSockets SendAudio", e.getMessage());
        }

    }

    public void stopThreads() {
        threadSendToServer.quitSafely();
        threadReceiveCommand.quitSafely();
        mThreadExecuteCommand.quitSafely();
        threadCheckDiconnection.quitSafely();
        try {
            threadSendToServer.join();
            threadSendToServer = null;
            handlerSendToServer = null;

            threadReceiveCommand.join();
            threadReceiveCommand = null;
            handlerReceiveCommand = null;

            mThreadExecuteCommand.join();
            mThreadExecuteCommand = null;
            mHandlerExecuteCommand = null;

            threadCheckDiconnection.join();
            threadCheckDiconnection = null;
            handlerCheckDiconnection = null;
        } catch (final InterruptedException e) {
            Log.e("Exception stopThreads", e.getMessage());
        }

    }
}
