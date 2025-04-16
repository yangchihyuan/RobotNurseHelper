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
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;

import ZenboNurseHelperProtobuf.ServerSend;
import tw.edu.cgu.ai.zenbo.env.Logger;

public class SocketManager {
    public String mServerURL;
    public Integer mPortNumber;

    public Socket mSocketReceiveResults;
    public Socket mSocketSendImages;
    public Socket mSocketSendAudio;

    private HandlerThread threadSendToServer;
    private Handler handlerSendToServer;
    private HandlerThread threadReceiveCommand;
    private Handler handlerReceiveCommand;
    private HandlerThread mThreadExecuteCommand;
    private Handler mHandlerExecuteCommand;
    private boolean mbReceiveCommand;

    byte[] mMessagePool = new byte[8192];
    int effective_length = 0;
    String beginString = new String("BeginOfAMessage");
    String endString = new String("EndOfAMessage");

    public RobotAPI mRobotAPI;
    ArrayList<ServerSend.ReportAndCommand> ArrayListCommand = new ArrayList<ServerSend.ReportAndCommand>();
    Converter converter;
    private static final Logger LOGGER = new Logger();

    public void startReceiveCommands()
    {
        handlerReceiveCommand.post(new Runnable() {
            @Override
            public void run() {
                mbReceiveCommand = true;
                while(mbReceiveCommand) {      //turn the boolean off when I need to turn off the thread
                    if (mSocketReceiveResults != null && mSocketReceiveResults.isConnected()) {
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

                                    ServerSend.ReportAndCommand report = ServerSend.ReportAndCommand.parseFrom(slice);
                                    //Do I need a mutex here to protect the ArrayList?
                                    ArrayListCommand.add(report);       //add to ArrayList
                                    //Post a Runnable here to execute the command?
                                    //Do I need a new thread here?
                                    mHandlerExecuteCommand.post(new Runnable() {
                                        @Override
                                        public void run() {
                                            if (!ArrayListCommand.isEmpty()) {
                                                ServerSend.ReportAndCommand report = ArrayListCommand.get(0);
                                                ArrayListCommand.remove(0);
                                                /*
                                                if (report.hasTimeStamp()) {
                                                    if (bMoveMode_LookForPeople) {
                                                        //                                    Log.d("report", report.toString());
                                                        m_DataBuffer.AddNewFrame(report);
                                                        mHandlerAction.post(mActionRunnable);
                                                    }
                                                }
                                                */
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
                                        }
                                    });
                                }
                            } else {
                                //sleep 30 msecs;
                                sleep(30);
                            }
                        } catch (Exception e) {
                            Log.e("Exception", e.getMessage());
                        }
                    }
                }
            }
        });
    }

    void sendImage(ByteBuffer imageAndData)
    {
        if( mSocketSendAudio != null && mSocketSendImages.isConnected()) {
            final boolean post = handlerSendToServer.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            OutputStream os = mSocketSendImages.getOutputStream();
                            os.write(imageAndData.array());

                        } catch (Exception e) {
                            Log.d("Exception Send to Server fails", e.getMessage()); //sendto failed: EPIPE (Broken pipe)
                        }
                    }
                }
            );
        }
    }

    void sendAudio(byte[] audioData)
    {
/*        final boolean post = mHandlerSendAudio.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            OutputStream os = mSocketSendAudio.getOutputStream();
                            os.write(audioData.array());

                        } catch (Exception e) {
                            Log.d("Exception Send to Server fails", e.getMessage()); //sendto failed: EPIPE (Broken pipe)
                        }
                    }
                }
        );
*/
        if( mSocketSendAudio != null && mSocketSendAudio.isConnected()) {
            try {
                OutputStream os = mSocketSendAudio.getOutputStream();
                os.write(audioData);

            } catch (Exception e) {
                Log.d("Exception Send to Server fails", e.getMessage()); //sendto failed: EPIPE (Broken pipe)
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
                    if(mSocketSendImages == null )
                        mSocketSendImages = new Socket();

                    SocketAddress socketAddress = new InetSocketAddress(mServerURL, mPortNumber);
                    mSocketSendImages.connect(socketAddress);

                    if( mSocketReceiveResults == null)
                        mSocketReceiveResults = new Socket();

                    SocketAddress socketAddress2 = new InetSocketAddress(mServerURL, mPortNumber+1);
                    mSocketReceiveResults.connect(socketAddress2);

                    if( mSocketSendAudio == null)
                        mSocketSendAudio =  new Socket();
                    SocketAddress socketAddress3 = new InetSocketAddress(mServerURL, mPortNumber+2);
                    mSocketSendAudio.connect(socketAddress3);
                } catch (Exception e) {
                    e.printStackTrace();
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
    }

    public void disconnectSockets()
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

    public void stopThreads() {
        threadSendToServer.quitSafely();
        threadReceiveCommand.quitSafely();
        mThreadExecuteCommand.quitSafely();
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

        } catch (final InterruptedException e) {
            LOGGER.e(e, "Exception!");
        }

    }
}
