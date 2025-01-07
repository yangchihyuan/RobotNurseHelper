/* Chih-Yuan Yang 2024/6/20: This is the file where I send commands to the Zenbo
SDK.
 */
package tw.edu.cgu.ai.zenbo;

import com.asus.robotframework.API.MotionControl;
import com.asus.robotframework.API.RobotFace;

import java.text.DecimalFormat;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

public class ActionRunnable implements Runnable {
    public int pitchDegree = 30;       //range -15 to 55
    public int yawDegree = 0;          //range -45(left) to 45(right)
    public com.asus.robotframework.API.RobotAPI ZenboAPI;
    public ZenboCallback robotCallback;
    private boolean bWaitingForRobotFinishesMovement = false;
    public boolean bDontMove = false;
    public boolean bDontRotateBody = false;
//    private MessageView mMessageView_Detection;
//    private MessageView mMessageView_Timestamp;
    private DataBuffer dataBuffer;
    public boolean mShowRobotFace = true;
    public enum RobotStatus
    {
        ACTIVE,IDLE
    }
    public enum DetectionMode {
        NOT_USED,
        NO_PERSON_IS_FOUND,
        NEGLECT_LOW_PROBABILITY,
        FALSE_POSITIVE,
        LONG_DISTANCE,
        HEAD_INVISIBLE_CENTER_HIPS_VISIBLE,
        NOSE_VISIBLE,
        BOTH_EARS_VISIBLE,
        LEFT_EAR_VISIBLE,
        RIGHT_EAR_VISIBLE,
        OTHERWISE,
        PEOPLE_ON_TV
    }

    private RobotStatus mRobotStatus = RobotStatus.ACTIVE;
    private int number_of_stuck = 0;
    private String mMessage_Detection = "";
    private String mMessage_Timestamp = "";
    private String newline = System.getProperty("line.separator");
    private String[] detection_mode_description = new String[12];
    private RobotFace mFaceStatus;
    private Timer mTimer_set_status_active;
    long idle_period = 15* 60 * 1000;  //15 min
    public boolean bKeepAlert = true;

    private boolean mbEyesNoseAllSeen;

    public class TimerTask_set_status_active extends TimerTask{
        public void run() {
            setRobotStatus(RobotStatus.ACTIVE);
            //mRobotStatus = RobotStatus.ACTIVE;
        }
    };


    public ActionRunnable() {
        bWaitingForRobotFinishesMovement = false;
        detection_mode_description[0]= "not used";
        detection_mode_description[1]= "no person is found";
        detection_mode_description[2]= "neglect, low probability";
        detection_mode_description[3]= "false positive";
        detection_mode_description[4]= "long distance";
        detection_mode_description[5]= "head invisible, center and 2 hips visible";
        detection_mode_description[6]= "nose visible";
        detection_mode_description[7]= "both ears visible";
        detection_mode_description[8]= "left ear visible";
        detection_mode_description[9]= "right ear visible";
        detection_mode_description[10]= "otherwise";
        detection_mode_description[11]= "neglect, person on TV";
    }

    /*
    public void setMessageView(MessageView MessageView_Detection, MessageView MessageView_Timestamp) {
        mMessageView_Detection = MessageView_Detection;
        mMessageView_Timestamp = MessageView_Timestamp;
        ZenboAPI.motion.moveHead(yawDegree, pitchDegree, MotionControl.SpeedLevel.Head.L3);
        robotCallback.RobotMovementFinished_Head = false;
        robotCallback.RobotMovementFinished_Body = false;
    }
*/

    public void setDataBuffer(DataBuffer dataBuffer) {
        this.dataBuffer = dataBuffer;
    }

    public RobotStatus getRobotStatus()
    {
        return mRobotStatus;
    }

    public void setRobotStatus(RobotStatus newRobotStatus)
    {
        if( newRobotStatus.equals(RobotStatus.ACTIVE)) {
            mRobotStatus = newRobotStatus;
            dataBuffer.BreakStandbyPattern();
            mTimer_set_status_active.cancel();
        }
    }

    @Override
    public void run() {
        boolean bContinue = true;
        final long timestamp_decision = System.currentTimeMillis();
        //2018/5/18 Chih-Yuan: Because the callback is unstable, I need to manually reset the flag
        if (robotCallback.RobotMovementFinished_Head == false && System.currentTimeMillis() - robotCallback.TimeStamp_MovementHead_Active > 3000) {
            robotCallback.RobotMovementFinished_Head = true;
        }
        if (robotCallback.RobotMovementFinished_Body == false && System.currentTimeMillis() - robotCallback.TimeStamp_MovementBody_Active > 4000) {
            robotCallback.RobotMovementFinished_Body = true;
        }

        if (robotCallback.RobotMovementFinished_Head && robotCallback.RobotMovementFinished_Body) {
            //sometimes I cannot get in this statement, when the floor is not even. Be careful.
            bWaitingForRobotFinishesMovement = false;
        }

        AnalyzedFrame LatestFrame = dataBuffer.getLatestFrame();
        //show messages of action recognition here
        int number_of_actions = LatestFrame.actions.length;
        mMessage_Timestamp = "";
        for(int i=0; i<number_of_actions ; i++)
            mMessage_Timestamp += (LatestFrame.actions[i] + newline);

        if (bWaitingForRobotFinishesMovement) {
            mMessage_Timestamp += "waiting for robot finishing a move";
            bContinue = false;
        }
        else if (dataBuffer.IsDataAvailable() == false) {
            mMessage_Timestamp += "data unavailable";
            bContinue = false;
        }
        else {
            long max_timestamp = dataBuffer.getLatestTimeStamp_OnImageAvailable();
            if (max_timestamp < robotCallback.TimeStamp_MovementFinished_Body ||
                    max_timestamp < robotCallback.TimeStamp_MovementFinished_Head) {
                mMessage_Timestamp += "max_timestamp less than robot movement finished";
                //It doesn't make sense to be continuously stuck in this if statement.
                //It appears that there is something wrong in Zenbo's callback function or the timestamp.
                number_of_stuck++;
                if (number_of_stuck > 10) {
                    //reset the two timestamps of the robotCallback
                    robotCallback.TimeStamp_MovementFinished_Body = 0;
                    robotCallback.TimeStamp_MovementFinished_Head = 0;
                }
                if (dataBuffer.IsDataFrozen() == false)
                    bContinue = false;
            } else {
                //reset the number
                number_of_stuck = 0;
            }
        }

//        mMessageView_Timestamp.setString(mMessage_Timestamp);

        if( bContinue) {
//            AnalyzedFrame LatestFrame = dataBuffer.getLatestFrame();
            //show messages of action recognition here
//            mMessage_Timestamp = LatestFrame.actions[0] + newline + LatestFrame.actions[1] + newline + LatestFrame.actions[2];
//            mMessageView_Timestamp.setString(mMessage_Timestamp);

            //Chih-Yuan Yang 2024/6/15: I do not use the AverageFrames object. Why do I need this statement?
            AverageFrame AverageFrames = dataBuffer.getAverageFrame();
            LatestFrame.bNew = false;
            float distance_d1811 = 0;
            DetectionMode detection_mode = DetectionMode.NOT_USED; //initialized
            int turn_direction = 0; //initialized
            int action_mode = 0;
            //action_mode 1: turn left
            //action_mode 2: turn right
            //action_mode 3: change pitch degree to 15
            //action_mode 4: move the robot's head to put the found person's P1811 center at the image center, and move the robot
            //action_mode 5: move the head up
            //action_mode 6: rotate the robot and adjust its head's pitch angle to track a subject's nose,
            //action_mode 7: random move, for break the pattern of turn twice

            //mRobotStatus 1: active
            //mRobotStatus 2: don't move to save energy

            if (LatestFrame.bFoundPerson == false) {
                detection_mode = DetectionMode.NO_PERSON_IS_FOUND;
            } else if (LatestFrame.bIgnorePerson) {
                detection_mode = DetectionMode.NEGLECT_LOW_PROBABILITY;
            }
            else {

                float[][] fMatrix = LatestFrame.fMatrix;
/*                List<int[]> yolo_coordinate_person = LatestFrame.yolo_coordinate_person;

                boolean bFalsePositive = true;
                for(int idx_yolo_person = 0; idx_yolo_person < LatestFrame.yolo_cnt_person ; idx_yolo_person++) {
                    int[] yolo_result = yolo_coordinate_person.get(idx_yolo_person);
                    int left = yolo_result[0];
                    int right = yolo_result[1];
                    int top = yolo_result[2];
                    int bottom = yolo_result[3];

                    int[] checkkeypoint = {0, 1, 8, 11}; //nose, center, left hip, right hip
                    //any one of the 4 points is covered by a person bounding box, we treat the openpose results as a true posible
                    for (int index : checkkeypoint) {
                        if (fMatrix[index][2] > 0) {
                            float coordinate_x = fMatrix[index][0];
                            float coordinate_y = fMatrix[index][1];
                            if (coordinate_x < right && coordinate_x > left && coordinate_y < bottom && coordinate_y > top) {
                                bFalsePositive = false;
                                break;
                            }
                        }
                    }
                }

 */

                //2019/4/26 Let me try temporally ignore this part. I want to know whether I can adjust
                //OpenPose's parameters to prevent false positives.
//                bFalsePositive = false;
//                if(bFalsePositive)
//                    detection_mode = DetectionMode.FALSE_POSITIVE;

/*
                List<int[]> yolo_coordinate_tvmonitor = LatestFrame.yolo_coordinate_tvmonitor;
                boolean bPersonOnTV = false;
                for(int idx_yolo_tvmonitor = 0; idx_yolo_tvmonitor < LatestFrame.yolo_cnt_tvmonitor ; idx_yolo_tvmonitor++) {
                    int[] yolo_result = yolo_coordinate_tvmonitor.get(idx_yolo_tvmonitor);
                    int left = yolo_result[0];
                    int right = yolo_result[1];
                    int top = yolo_result[2];
                    int bottom = yolo_result[3];

                    int[] checkkeypoint = {1}; //center
                    //if the center point is inside a TV bounding box, treat it as negative
                    for (int index : checkkeypoint) {
                        if (fMatrix[index][2] > 0) {
                            float coordinate_x = fMatrix[index][0];
                            float coordinate_y = fMatrix[index][1];
                            if (coordinate_x < right && coordinate_x > left && coordinate_y < bottom && coordinate_y > top) {
                                bPersonOnTV = true;
                                break;
                            }
                        }
                    }
                }
                if(bPersonOnTV)
                    detection_mode = DetectionMode.PEOPLE_ON_TV;
*/

                if( fMatrix[2][2] > 0 && fMatrix[14][2] >0 && fMatrix[15][2] > 0)
                    mbEyesNoseAllSeen = true;
                else
                    mbEyesNoseAllSeen = false;
            }


            if(detection_mode == DetectionMode.NOT_USED)     //default mode
            {
                float[][] fMatrix = LatestFrame.fMatrix;
                //if center and two hips are visible, but all keypoints on the head are invisible, the robot should raise its head.
                if (fMatrix[1][2] > 0 && fMatrix[8][2] > 0 && fMatrix[11][2] > 0 &&
                        fMatrix[0][2] == 0 && fMatrix[14][2] == 0 && fMatrix[15][2] == 0 && fMatrix[16][2] == 0 && fMatrix[17][2] == 0) {
                    detection_mode = DetectionMode.HEAD_INVISIBLE_CENTER_HIPS_VISIBLE;
                }
                else if (fMatrix[0][2] > 0)       //fMatrix[0][2] is the probability of the point of nose.
                    detection_mode = DetectionMode.NOSE_VISIBLE;
                else if (fMatrix[16][2] > 0 && fMatrix[17][2] > 0) //Sometimes the nose is invisible. //fMatrix[16] and [17] are the two ears.
                    detection_mode = DetectionMode.BOTH_EARS_VISIBLE;
                else if (fMatrix[16][2] == 0 && fMatrix[17][2] > 0)     //[17]: left ear
                    detection_mode = DetectionMode.LEFT_EAR_VISIBLE;
                else if ( fMatrix[16][2] > 0 && fMatrix[17][2] == 0 )
                    detection_mode = DetectionMode.RIGHT_EAR_VISIBLE;
                else
                    detection_mode = DetectionMode.OTHERWISE;

                float chest_x = fMatrix[1][0];
                float chest_y = fMatrix[1][1];
                float lefthip_x = fMatrix[8][0];
                float lefthip_y = fMatrix[8][1];
                float righthip_x = fMatrix[11][0];
                float righthip_y = fMatrix[11][1];
                mRobotStatus = RobotStatus.ACTIVE;
                float distance_chest_to_lefthip = (float) Math.sqrt((Math.pow(chest_x - lefthip_x, 2) + Math.pow(chest_y - lefthip_y, 2)));
                float distance_chest_to_righthip = (float) Math.sqrt((Math.pow(chest_x - righthip_x, 2) + Math.pow(chest_y - righthip_y, 2)));
                if( fMatrix[1][2] == 1 && fMatrix[8][2] == 1 && fMatrix[11][2] == 1)
                    distance_d1811 = (distance_chest_to_lefthip + distance_chest_to_righthip) / 2;
                else
                    distance_d1811 = 1000;      //mean the value is not valid
            }

            DecimalFormat converter = new DecimalFormat("0");
            int detection_mode_value = 0;
            switch(detection_mode)
            {
                case NO_PERSON_IS_FOUND:
                    detection_mode_value = 1;
                    break;
                case NEGLECT_LOW_PROBABILITY:
                    detection_mode_value = 2;
                    break;
                case FALSE_POSITIVE:
                    detection_mode_value = 3;
                    break;
                case LONG_DISTANCE:
                    detection_mode_value = 4;
                    break;
                case HEAD_INVISIBLE_CENTER_HIPS_VISIBLE:
                    detection_mode_value = 5;
                    break;
                case NOSE_VISIBLE:
                    detection_mode_value = 6;
                    break;
                case BOTH_EARS_VISIBLE:
                    detection_mode_value = 7;
                    break;
                case LEFT_EAR_VISIBLE:
                    detection_mode_value = 8;
                    break;
                case RIGHT_EAR_VISIBLE:
                    detection_mode_value = 9;
                    break;
                case OTHERWISE:
                    detection_mode_value = 10;
                    break;
                case PEOPLE_ON_TV:
                    detection_mode_value = 11;
                    break;
            }


            mMessage_Detection = "detection_mode:" + Integer.toString(detection_mode_value) + " " +
                    detection_mode_description[detection_mode_value] +" d1818=" +converter.format(distance_d1811) +
                    " pitch:" + Integer.toString(pitchDegree) + " yaw:" + Integer.toString(yawDegree);

            if( bDontRotateBody && bDontMove)  //move head only
            {
                float[][] fMatrix = LatestFrame.fMatrix;
                float x = 320;
                float y = 160;
                boolean bMoveHead = false;
                if( detection_mode == DetectionMode.NO_PERSON_IS_FOUND || detection_mode == DetectionMode.FALSE_POSITIVE || detection_mode == DetectionMode.NEGLECT_LOW_PROBABILITY ) {
                    if( yawDegree != 0) {
                        yawDegree = 0;
                        bMoveHead = true;
                    }
                    if( pitchDegree != 15)
                    {
                        pitchDegree = 15;
                        bMoveHead = true;
                    }
                }
                else if(detection_mode == DetectionMode.LONG_DISTANCE || detection_mode == DetectionMode.HEAD_INVISIBLE_CENTER_HIPS_VISIBLE) {
                    x = fMatrix[1][0];
                    y = fMatrix[1][1];
                }
                else if( detection_mode == DetectionMode.NOSE_VISIBLE )
                {
                    y = fMatrix[0][1];
                    x = fMatrix[0][0];
                }
                else if( detection_mode == DetectionMode.BOTH_EARS_VISIBLE ){
                    y = (fMatrix[16][1] + fMatrix[17][1])/2;        //the center of the 2 ears on the y axis
                    x = (fMatrix[16][0] + fMatrix[17][0])/2;    // the center of the 2 ears on the x axis
                }
                else if( detection_mode == DetectionMode.LEFT_EAR_VISIBLE )
                {
                    y = fMatrix[17][1];
                    x = fMatrix[17][0];
                }
                else if( detection_mode == DetectionMode.RIGHT_EAR_VISIBLE) {
                    y = fMatrix[16][1];
                    x = fMatrix[16][0];
                }
                else if( detection_mode == DetectionMode.OTHERWISE )
                {
/*
                    if( LatestFrame.tracker_roi_x != -1)
                    {
                        x = (LatestFrame.tracker_roi_x + LatestFrame.tracker_roi_width)/2;
                        y = (LatestFrame.tracker_roi_y + LatestFrame.tracker_roi_height)/2;
                    }
 */
                }
                int rotate_degree = Math.round((x - 320) / 10.2f );  //640 * 62.5053f);
                int tilt_degree = Math.round((y - 160) / 10.2f);

                if (tilt_degree > 5 || tilt_degree < -5 ) {
                    pitchDegree = pitchDegree - tilt_degree;        //range -15 to 55, Zenbo's vertical view angle is 48.9336
                    if (pitchDegree > 55)
                        pitchDegree = 55;
                    if (pitchDegree < -15)
                        pitchDegree = -15;
                    bMoveHead = true;
                }

                if (rotate_degree > 5 || rotate_degree < -5 ) {
                    yawDegree = yawDegree - rotate_degree;
                    if (yawDegree > 45)
                        yawDegree = 45;
                    if (yawDegree < -45)
                        yawDegree = -45;
                    bMoveHead = true;
                }

                if( bMoveHead) {
                    ZenboAPI.motion.moveHead(yawDegree, pitchDegree, MotionControl.SpeedLevel.Head.L3);
                    bWaitingForRobotFinishesMovement = true;
                    robotCallback.RobotMovementFinished_Head = false;
                    robotCallback.TimeStamp_MovementFinished_Head = Long.MAX_VALUE;
                }
            }
            else        //move body
            {
                if( yawDegree != 0)
                    yawDegree = 0;

                if (mRobotStatus.equals(RobotStatus.ACTIVE) && (detection_mode == DetectionMode.NO_PERSON_IS_FOUND || detection_mode == DetectionMode.NEGLECT_LOW_PROBABILITY)) {
                    boolean bTurnOneAround = dataBuffer.CheckTurnOneAround();
                    boolean bTurnTwoAround = dataBuffer.CheckTurnTwoAround();
                    //I need to reset the dataBuffer if I want Zenbo to move again.
                    if (bTurnTwoAround) {
                        if (bKeepAlert) {
                            action_mode = 7;
                            dataBuffer.AddAnAction(action_mode);
                            //move to a random location

                            float relocate_y = (float) Math.random() * 2 - 1;       //range -1 to 1
                            float relocate_x = (float) Math.random() * 2 - 1;       //range -1 to 1
                            int rotate_degree = Math.round((float)Math.random() * 360 - 180);         //range -180 to 180

                            if (relocate_x != 0 || relocate_y != 0 || rotate_degree != 0) {
                                ZenboAPI.motion.moveBody(relocate_x, relocate_y, rotate_degree);
                                bWaitingForRobotFinishesMovement = true;
                                robotCallback.RobotMovementFinished_Body = false;
                                robotCallback.TimeStamp_MovementFinished_Body = Long.MAX_VALUE;
                            }

                        } else {
                            mRobotStatus = RobotStatus.IDLE;
                            //launch a timer
                            mTimer_set_status_active = new Timer();
                            TimerTask_set_status_active task = new TimerTask_set_status_active();
                            mTimer_set_status_active.schedule(task, idle_period, idle_period);
                        }
                    } else if (bTurnOneAround) {
                        //change pitch degree
                        pitchDegree = 15;
                        ZenboAPI.motion.moveHead(yawDegree, pitchDegree, MotionControl.SpeedLevel.Head.L3);
                        bWaitingForRobotFinishesMovement = true;
                        robotCallback.RobotMovementFinished_Head = false;
                        robotCallback.TimeStamp_MovementFinished_Head = Long.MAX_VALUE;
                        action_mode = 3;
                        dataBuffer.AddAnAction(action_mode);
                    } else {
                        float relocate_y = 0;
                        float relocate_x = 0;
                        turn_direction = dataBuffer.CheckMostRecentData_PersonAtLeftOrRight();
                        int rotate_degree = 0;
                        if (turn_direction == 2)        //turn_direction == 2 means turn left
                        {
                            rotate_degree = 30;
                            action_mode = 1;
                            dataBuffer.AddAnAction(action_mode);
                        } else if (turn_direction == 1)   //turn_direction == 1 means turn right
                        {
                            rotate_degree = -30;
                            action_mode = 2;
                            dataBuffer.AddAnAction(action_mode);
                        } else if (turn_direction == 0)        //the robot hasn't seen any person yet
                        {
                            rotate_degree = -30;
                            action_mode = 2;
                            dataBuffer.AddAnAction(action_mode);
                        }

                        if (rotate_degree != 0) {
                            ZenboAPI.motion.moveBody(relocate_x, relocate_y, rotate_degree);
                            bWaitingForRobotFinishesMovement = true;
                            robotCallback.RobotMovementFinished_Body = false;
                            robotCallback.TimeStamp_MovementFinished_Body = Long.MAX_VALUE;
                        }
                    }
                }

                if (detection_mode == DetectionMode.HEAD_INVISIBLE_CENTER_HIPS_VISIBLE) {          //head invisible, center and 2 hips visible
                    action_mode = 5;
                    float[][] fMatrix = LatestFrame.fMatrix;
                    float chest_y = fMatrix[1][1];
                    dataBuffer.AddAnAction(action_mode);
                    float expected_nose_y = chest_y - 50;
                    float vertical_center_shift = expected_nose_y - 240;
                    float vertical_center_shift_degree = vertical_center_shift / 10.2f;     //the value 10.2 is computed from experiments
                    pitchDegree = pitchDegree - Math.round(vertical_center_shift_degree);        //range -15 to 55, Zenbo's vertical view angle is 48.9336
                    if (pitchDegree > 55)
                        pitchDegree = 55;
                    if (pitchDegree < -15)
                        pitchDegree = -15;

                    ZenboAPI.motion.moveHead(yawDegree, pitchDegree, MotionControl.SpeedLevel.Head.L3);
                    bWaitingForRobotFinishesMovement = true;
                    robotCallback.RobotMovementFinished_Head = false;
                    robotCallback.TimeStamp_MovementFinished_Head = Long.MAX_VALUE;
                }

                //Track nose or ears
                if( detection_mode == DetectionMode.NOSE_VISIBLE || detection_mode == DetectionMode.BOTH_EARS_VISIBLE ||
                        detection_mode == DetectionMode.LEFT_EAR_VISIBLE || detection_mode == DetectionMode.RIGHT_EAR_VISIBLE) {
                    action_mode = 6;
                    dataBuffer.AddAnAction(action_mode);
                    float[][] fMatrix = LatestFrame.fMatrix;
                    float x=0,y=0;

                    if (detection_mode == DetectionMode.NOSE_VISIBLE) {
                        y = fMatrix[0][1];
                        x = fMatrix[0][0];
                    }
                    else if (detection_mode == DetectionMode.BOTH_EARS_VISIBLE) {                 //Track 2 ears
                        y = (fMatrix[16][1] + fMatrix[17][1])/2;        //the center of the 2 ears on the y axis
                        x = (fMatrix[16][0] + fMatrix[17][0])/2;    // the center of the 2 ears on the x axis
                    }
                    else if (detection_mode == DetectionMode.LEFT_EAR_VISIBLE) {                  //Track the left ear
                        y = fMatrix[17][1];
                        x = fMatrix[17][0];
                    }
                    else if( detection_mode == DetectionMode.RIGHT_EAR_VISIBLE) {                  //Track the right ear
                        y = fMatrix[16][1];
                        x = fMatrix[16][0];
                    }

                    int tilt_degree = Math.round((y - 160) / 10.2f);
                    int rotate_degree = Math.round(-(x - 320) / 10.2f);
                    boolean bMoveHead = false;
                    if (tilt_degree > 5 || tilt_degree < -5 ) {
                        pitchDegree = pitchDegree - tilt_degree;        //range -15 to 55, Zenbo's vertical view angle is 48.9336
                        if (pitchDegree > 55)
                            pitchDegree = 55;
                        if (pitchDegree < -15)
                            pitchDegree = -15;
                        bMoveHead = true;
                        ZenboAPI.motion.moveHead(yawDegree, pitchDegree, MotionControl.SpeedLevel.Head.L3);
                    }

                    boolean bMoveBody = false;
                    if( distance_d1811 < 131.7489 )
                    {
                        float distance_forward = 0;
                        if (distance_d1811 >= 96.9894 && distance_d1811 < 131.7489) {       //not very far away, how much should the robot move?
                            distance_forward = 0.5f;
                        } else if (distance_d1811 >= 73.1422 && distance_d1811 < 96.9894) {
                            distance_forward = 1.0f;
                        } else { //long distance
                            distance_forward = 1.5f;
                        }

                        action_mode = 4;
                        dataBuffer.AddAnAction(action_mode);


                        if (bDontMove)      //a special flag to record training data
                            distance_forward = 0;

                        float rotate_radian = rotate_degree / 180f * 3.14f;     //bug: int / 180(int) becomes 0

                        //from the relativeThetaDegree and distance_forward, to compute the new position

                        float relocate_y = distance_forward * (float) Math.sin(rotate_radian);
                        float relocate_x = distance_forward * (float) Math.cos(rotate_radian);
                        if (relocate_x != 0 || relocate_y != 0 || rotate_radian != 0) {
                            bMoveBody = true;
                            ZenboAPI.motion.moveBody(relocate_x, relocate_y, rotate_degree);
                        }
                    }
                    else        //short distance, the robot doesn't need to move
                    {
                        if( rotate_degree > 5 || rotate_degree < -5) {
                            ZenboAPI.motion.moveBody(0.0f, 0.0f, rotate_degree);
                            bMoveBody = true;
                        }
                    }

                    if( bMoveHead || bMoveBody ) {
                        bWaitingForRobotFinishesMovement = true;
                        robotCallback.RobotMovementFinished_Head = false;
                        robotCallback.TimeStamp_MovementFinished_Head = Long.MAX_VALUE;
                    }

                }

                //use tracker's information
                if( detection_mode == DetectionMode.OTHERWISE )
                {
                    action_mode = 6;
                    dataBuffer.AddAnAction(action_mode);
                    float x=0,y=0;
                    /*
                    if( LatestFrame.tracker_roi_x != -1)
                    {
                        x = 0.5f*(LatestFrame.tracker_roi_x + LatestFrame.tracker_roi_width);
                        y = 0.5f*(LatestFrame.tracker_roi_y + LatestFrame.tracker_roi_height);

                        int tilt_degree = Math.round((y - 160) / 10.2f);
                        int rotate_degree = Math.round(-(x - 320) / 10.2f);
                        boolean bMoveHead = false;
                        if (tilt_degree > 5 || tilt_degree < -5 ) {
                            pitchDegree = pitchDegree - tilt_degree;        //range -15 to 55, Zenbo's vertical view angle is 48.9336
                            if (pitchDegree > 55)
                                pitchDegree = 55;
                            if (pitchDegree < -15)
                                pitchDegree = -15;
                            bMoveHead = true;
                            ZenboAPI.motion.moveHead(yawDegree, pitchDegree, MotionControl.SpeedLevel.Head.L3);
                        }
                        boolean bMoveBody = false;
                        if( distance_d1811 < 131.7489 )
                        {
                            float distance_forward = 0;
                            if (distance_d1811 >= 96.9894 && distance_d1811 < 131.7489) {       //not very far away, how much should the robot move?
                                distance_forward = 0.5f;
                            } else if (distance_d1811 >= 73.1422 && distance_d1811 < 96.9894) {
                                distance_forward = 1.0f;
                            } else { //long distance
                                distance_forward = 1.5f;
                            }

                            action_mode = 4;
                            dataBuffer.AddAnAction(action_mode);


                            if (bDontMove)      //a special flag to record training data
                                distance_forward = 0;

                            float rotate_radian = rotate_degree / 180f * 3.14f;     //bug: int / 180(int) becomes 0

                            //from the relativeThetaDegree and distance_forward, to compute the new position

                            float relocate_y = distance_forward * (float) Math.sin(rotate_radian);
                            float relocate_x = distance_forward * (float) Math.cos(rotate_radian);
                            if (relocate_x != 0 || relocate_y != 0 || rotate_radian != 0) {
                                bMoveBody = true;
                                ZenboAPI.motion.moveBody(relocate_x, relocate_y, rotate_degree);
                            }
                        }
                        else        //short distance, the robot doesn't need to move
                        {
                            if( rotate_degree > 5 || rotate_degree < -5) {
                                ZenboAPI.motion.moveBody(0.0f, 0.0f, rotate_degree);
                                bMoveBody = true;
                            }
                        }

                        if( bMoveHead || bMoveBody ) {
                            bWaitingForRobotFinishesMovement = true;
                            robotCallback.RobotMovementFinished_Head = false;
                            robotCallback.TimeStamp_MovementFinished_Head = Long.MAX_VALUE;
                        }

                    }

                     */
                }

            }


            RobotFace newExpression = RobotFace.AWARE_LEFT; //not used, just for initialization.
            if (mShowRobotFace) {
                switch (detection_mode_value) {
                    case 1:
                    case 2:
                    case 3:
                        if( turn_direction == 2 ) {
                            newExpression = RobotFace.AWARE_LEFT;
                        }
                        else {
                            newExpression = RobotFace.AWARE_RIGHT;
                        }
                        break;
                    case 4:
                        newExpression = RobotFace.ACTIVE;
                        break;
                    case 5:
                        newExpression = RobotFace.QUESTIONING;
                        break;
                    case 6:
                        newExpression = RobotFace.HAPPY;
                        break;
                    case 7:
                    case 8:
                    case 9:
                        newExpression = RobotFace.EXPECTING;
                        break;
                }
            }
            else {
                newExpression = RobotFace.HIDEFACE;
            }

            //TODO: there is something wrong here, the face may not appear. I had better to set a face if mShowRobotFace is true.
            if( !(newExpression.equals(mFaceStatus)) ) {
                ZenboAPI.robot.setExpression(newExpression);
                mFaceStatus = newExpression;
            }
//            mMessageView_Detection.setString(mMessage_Detection);
        }
    }//end of run

}
