package tw.edu.cgu.ai.zenbo;

import static java.lang.Thread.sleep;

import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import com.asus.robotframework.API.ExpressionConfig;
import com.asus.robotframework.API.MotionControl;
import com.asus.robotframework.API.RobotAPI;
import com.asus.robotframework.API.RobotFace;
import com.asus.robotframework.API.SpeakConfig;

import java.io.BufferedInputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;

import ZenboNurseHelperProtobuf.ServerSend;

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

    public RobotAPI mRobotAPI;
    ArrayList<ServerSend.ReportAndCommand> ArrayListCommand = new ArrayList<ServerSend.ReportAndCommand>();
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

                                    ServerSend.ReportAndCommand report = ServerSend.ReportAndCommand.parseFrom(slice);
                                    Log.d("Debug", "Receive a message");
                                    if (report.hasX()) {
                                        Log.d("move body", report.toString());
                                        int serial = mRobotAPI.motion.moveBody(((float) report.getX()) / 100.0f, ((float) report.getY()) / 100.0f, report.getDegree());
                                        //The serial number will appear in the callback function.
                                    }
                                    //rotate only
                                    if (!report.hasX() && !report.hasY() && report.hasDegree()) {
                                        Log.d("rotate body", report.toString());
                                        int serial = mRobotAPI.motion.moveBody(0.0f, 0.0f, report.getDegree());
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
                                        RobotFace newFace = converter.FaceIndexToRobotFace(report.getFace());
                                        ExpressionConfig config = new ExpressionConfig();
                                        config.volume(report.getVolume()).speed(report.getSpeed()).pitch(report.getSpeakPitch());
                                        mRobotAPI.robot.setExpression(newFace, report.getSpeakSentence(), config);
                                    } else if (report.hasSpeakSentence()) {
                                        Log.d("Speak Sentence", report.getSpeakSentence());
                                        SpeakConfig config = new SpeakConfig();
                                        config.volume(report.getVolume()).speed(report.getSpeed()).pitch(report.getSpeakPitch());
                                        mRobotAPI.robot.speak(report.getSpeakSentence(), config);
                                    } else if (report.hasFace()) {
                                        RobotFace newFace = converter.FaceIndexToRobotFace(report.getFace());
                                        mRobotAPI.robot.setExpression(newFace);
                                    }

                                    if (report.hasStopmove()) {
                                        mRobotAPI.motion.stopMoving();   //this function does not work.
                                    }
                                    if (report.hasPredefinedAction()) {
                                        //it will still return a serial, but for loop action, will the onResult() in the CallBack be called?
                                        int serial = mRobotAPI.utility.playAction(converter.PredefinedActionIndexToPlayAction(report.getPredefinedAction()));
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

    public void enableAutoReconnection()
    {
        bAutoReconnection = true;
        handlerCheckDiconnection.post(new Runnable() {
            @Override
            public void run() {
                while(bAutoReconnection) {
                    Log.d("autoReconnection", "enableAutoReconnection");
                    if( mSocketSendImages == null)
                        connectSockets();
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
            socketReceiveCommand.close();
            mSocketSendAudio.close();
        } catch (Exception e) {
            Log.e("disconnectSockets Exception", e.getMessage());
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
