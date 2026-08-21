
package tw.edu.cgu.ai;

import android.util.Log;

import com.asus.robotframework.API.RobotCallback;
import com.asus.robotframework.API.RobotCmdState;
import com.asus.robotframework.API.RobotErrorCode;
import com.asus.robotframework.API.RobotCommand;

import com.google.protobuf.Timestamp;
import org.json.JSONObject;

public class ZenboCallback extends RobotCallback implements RobotCallback.Listen {

    public SocketManager socketManager;

    public boolean RobotMovementFinished_Head = true;
    public boolean RobotMovementFinished_Body = true;
    public long TimeStamp_MovementFinished_Head = Long.MIN_VALUE;
    public long TimeStamp_MovementFinished_Body = Long.MIN_VALUE;
    public long TimeStamp_MovementHead_Active = 0;      //can not use Long.Max_Value, out of range after the subtraction.
    public long TimeStamp_MovementBody_Active = 0;

    @Override
    public void onStateChange(int cmd, int serial, RobotErrorCode err_code, RobotCmdState state) {
        //check, what will happen is the power core is plugged?
        Log.d("ZenboCallback", "onStateChange " + String.format("cmd %d serial %d ",cmd, serial) + "err_code " + err_code.toString() + " state " + state.toString());
        if (cmd == RobotCommand.MOTION_MOVE_BODY.getValue()) {        //cmd == 39 movebody
            if (state == RobotCmdState.ACTIVE) {
                //It happen this callback is erased by the Zenbo system prompt.
                RobotMovementFinished_Body = false;
                TimeStamp_MovementBody_Active = System.currentTimeMillis();
            } else if (state == RobotCmdState.SUCCEED) {
                RobotMovementFinished_Body = true;
                TimeStamp_MovementFinished_Body = System.currentTimeMillis();
            } else if (state == RobotCmdState.PENDING) {
                //When does it happen? When Zenbo finds an obstacle.
                RobotMovementFinished_Body = false;
            } else if (state == RobotCmdState.REJECTED) {
                //it happens when Zenbo is updating its language dataset.
                //or the power core is plugged, the RobotCmdState.SUCCEED will never come.
                RobotMovementFinished_Body = true;
                TimeStamp_MovementFinished_Body = System.currentTimeMillis();
            } else if (state == RobotCmdState.FAILED){
                //it happens when Zenbo fails to move to the expected location
                RobotMovementFinished_Body = true;
                TimeStamp_MovementFinished_Body = System.currentTimeMillis();
            } else if (state == RobotCmdState.INITIAL) {
                //I never saw this state, maybe this state is language-related.
                RobotMovementFinished_Body = false;
            }else{
                RobotMovementFinished_Body = false;
            }

        }

        if (cmd == RobotCommand.MOVE_HEAD.getValue())        //cmd == 6, moveHead
        {
            //2018/5/28 Chih-Yuan: The callback is unstable. It happens that there is no more callback after ACTIVE
            if (state == RobotCmdState.ACTIVE) {
                RobotMovementFinished_Head = false;
                TimeStamp_MovementHead_Active = System.currentTimeMillis();
            } else if (state == RobotCmdState.SUCCEED) {
                RobotMovementFinished_Head = true;
                TimeStamp_MovementFinished_Head = System.currentTimeMillis();
            } else if (state == RobotCmdState.PENDING) {
                //it happens when Zenbo is waiting the user's permit to move. However, will Zenbo call the onResult() function?
                //2018/5/28: There is a bug. Zenbo won't send another state after sending this state. Thus I have to set this to true;
                RobotMovementFinished_Head = true;
                TimeStamp_MovementFinished_Head = System.currentTimeMillis();
            } else if (state == RobotCmdState.REJECTED) {
                //it happens when Zenbo is updating its language dataset.
                RobotMovementFinished_Head = true;
                TimeStamp_MovementFinished_Head = System.currentTimeMillis();
            } else if (state == RobotCmdState.FAILED){
                //it happens along with an err_code: MOTION_OUT_OF_BOUNDS, if we use an argument out of range
                RobotMovementFinished_Body = true;
                TimeStamp_MovementFinished_Head = System.currentTimeMillis();
            } else {
                TimeStamp_MovementFinished_Head = System.currentTimeMillis();
                RobotMovementFinished_Head = true;
            }
        }

        /*Command Name           Integer Value (getValue())
          SET_EXPRESSION         31
          SPEAK                  33
          STOP_SPEAK             34
          MOTION_MOVE_BODY       39
          SPEAK_FROM_DS          104
         */
        if( cmd == RobotCommand.SPEAK.getValue())
        {
            Log.d("RobotCommand.SPEAK", "onStateChange: cmd = " + cmd + " serial = " + serial + " err_code = " + err_code.toString() + " state = " + state.toString());
            if( state == RobotCmdState.SUCCEED)
            {
                //Send a command to Server
                if (socketManager != null && socketManager.mSocketSendMessages != null) {
                    long timestamp = System.currentTimeMillis();
                    Timestamp eventTime = Timestamp.newBuilder()
                            .setSeconds(timestamp / 1000)
                            .setNanos((int) ((timestamp % 1000) * 1000000))
                            .build();

                    RobotCommandOuterClass.RobotToServerMessage message =
                            RobotCommandOuterClass.RobotToServerMessage.newBuilder()
                                    .setEventTime(eventTime)
                                    .setDescription("onTTSComplete")
                                    .build();

                    socketManager.sendAMessage(message, socketManager.mSocketSendMessages);
                }

            }
        }
        super.onStateChange(cmd, serial, err_code, state);
    }

    //
    @Override
    public void onResult(int cmd,
                         int serial,
                         RobotErrorCode err_code,
                         android.os.Bundle result)
    {
        //Check this. If I can catch the result of the speak() function.
        Log.d("ZenboCallback", "onResult: cmd " + Integer.toString(cmd));
        super.onResult(cmd, serial, err_code, result);

    }

    @Override
    public void onFinishRegister() {

    }

    @Override
    public void onVoiceDetect(JSONObject jsonObject) {

    }

    @Override
    public void onSpeakComplete(String s, String s1) {
        Log.d("ZenboCallback", "onSpeakComplete: " + s);
        //Send a command to Server
        if (socketManager != null && socketManager.mSocketSendMessages != null) {
            long timestamp = System.currentTimeMillis();
            Timestamp eventTime = Timestamp.newBuilder()
                    .setSeconds(timestamp / 1000)
                    .setNanos((int) ((timestamp % 1000) * 1000000))
                    .build();

            RobotCommandOuterClass.RobotToServerMessage message =
                    RobotCommandOuterClass.RobotToServerMessage.newBuilder()
                            .setEventTime(eventTime)
                            .setDescription("onTTSComplete")
                            .build();

            socketManager.sendAMessage(message, socketManager.mSocketSendMessages);
            Log.d("ZenboCallback", "Send onSpeakComplete: " + s);
        }
    }

    @Override
    public void onEventUserUtterance(JSONObject jsonObject) {

    }

    @Override
    public void onResult(JSONObject jsonObject) {
        Log.d("ZenboCallback", "onResult: " + jsonObject.toString());

    }

    @Override
    public void onRetry(JSONObject jsonObject) {

    }
}
