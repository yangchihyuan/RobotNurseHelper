package tw.edu.cgu.ai.zenbo;

import android.util.Log;
import java.util.LinkedList;
import static java.lang.Math.max;
import ZenboNurseHelperProtobuf.ServerSend.ReportAndCommand;


public class DataBuffer {
    AnalyzedFrame[] AnalyzedFrameArray;
    int[] ActionArray;
    private boolean bFreezeData = false;
    private int frame_buffer_index = -1;
    private int action_buffer_index = -1;
    private int mLastPersonLocationLeftOrRight = -1;
    private boolean m_bPersonLocationReset = true;

    public boolean IsDataAvailable() {
        if (frame_buffer_index >= 0)
            return true;
        else
            return false;
    }

    //constructor
    public DataBuffer(int number) {
        SetSize(number);
    }

    public void SetSize(int number) {
        AnalyzedFrameArray = new AnalyzedFrame[number];
        for (int i = 0; i < AnalyzedFrameArray.length; i++)
            AnalyzedFrameArray[i] = new AnalyzedFrame();

        ActionArray = new int[number];      //default 0

    }

    public boolean IsDataFrozen() {
        return bFreezeData;
    }

    public void FreezeData() {
        bFreezeData = true;
    }

    public void UnfreezeData() {
        bFreezeData = false;
    }

    public void AddNewFrame(ReportAndCommand report) {
        int new_index = frame_buffer_index + 1;
        if (new_index == AnalyzedFrameArray.length)
            new_index = 0;
        AnalyzedFrameArray[new_index].timestamp_ReceivedFromServer = System.currentTimeMillis();
        AnalyzedFrameArray[new_index].ParseServerReturn(report);
        boolean bshow_process_time = false;
        if( bshow_process_time)
            Log.d("Process time", String.valueOf( AnalyzedFrameArray[new_index].timestamp_ReceivedFromServer - AnalyzedFrameArray[new_index].timestamp_OnImageAvailable));
        AnalyzedFrameArray[new_index].bNew = true;
        if (AnalyzedFrameArray[new_index].bFoundPerson)
            m_bPersonLocationReset = true;
        AnalyzedFrameArray[new_index].bAvailable = true;

        //update the variable frame_buffer_index when everything is ready. Otherwise, the getLatestFrame() function
        //may return a frame with a null action field.
        frame_buffer_index = new_index;
    }

    public float[][] getLatestfMatrix() {
        return AnalyzedFrameArray[frame_buffer_index].fMatrix;
    }

    public void AddAnAction(int action_mode) {
        action_buffer_index++;
        if (action_buffer_index == ActionArray.length)
            action_buffer_index = 0;
        ActionArray[action_buffer_index] = action_mode;
    }

//    public float[] getLatestyMatrix() {
//        return AnalyzedFrameArray[frame_buffer_index].yMatrix;
//    }

    public long getLatestTimeStamp_OnImageAvailable() {
        return AnalyzedFrameArray[frame_buffer_index].timestamp_OnImageAvailable;
    }

    public AnalyzedFrame getLatestFrame() {
        return AnalyzedFrameArray[frame_buffer_index];
    }

    //2025/1/8 I did not use this function
/*
    public AverageFrame getAverageFrame() {
        AverageFrame result = new AverageFrame();
        int number_of_checked_frames = 3;
        LinkedList<Integer> list = new LinkedList<>();
        int index = frame_buffer_index - 1;
        int lower_bound = max(0, frame_buffer_index - number_of_checked_frames +1);
        for (; index >= lower_bound; index--) {
            list.add(index);
        }
        if (frame_buffer_index +1 < number_of_checked_frames) {
            for (index = AnalyzedFrameArray.length - 1; index >= AnalyzedFrameArray.length - number_of_checked_frames + frame_buffer_index; index--) {
                list.add(index);
            }
        }

        int[] checkset_1811 = {1, 8, 11};
        result.bValid_1811 = true;
        while( !list.isEmpty())
        {
            index = list.poll();
            for(int i=0; i<18; i++)
            {
                for( int j=0; j<3; j++)
                {
                    result.fMatrix[i][j] += AnalyzedFrameArray[index].fMatrix[i][j];
                }
            }
            for( int j=0; j<checkset_1811.length-1; j++)
            {
                int i = checkset_1811[j];
                if( AnalyzedFrameArray[index].fMatrix[i][2] == 0 ) {
                    result.bValid_1811 = false;
                    break;
                }
            }
        }
        for(int i=0; i<18; i++)
        {
            for( int j=0; j<3; j++)
            {
                result.fMatrix[i][j] /= number_of_checked_frames;
            }
        }

        return result;
    }
*/
    /*1: person at the righthand side
      2: person at the lefthand side
    */
    public int CheckMostRecentData_PersonAtLeftOrRight() {
        if (m_bPersonLocationReset) {
            int index_search = frame_buffer_index - 1;
            boolean bFound = false;
            int PersonLocation = 0;
            for (; index_search > 0; index_search--) {
                if (AnalyzedFrameArray[index_search].bFoundPerson) {
                    //check the left or right side
                    float x_nose = AnalyzedFrameArray[index_search].fMatrix[0][0];
                    float x_chest = AnalyzedFrameArray[index_search].fMatrix[1][0];
                    float x_lefthip = AnalyzedFrameArray[index_search].fMatrix[8][0];
                    float x_righthip = AnalyzedFrameArray[index_search].fMatrix[11][0];
                    if (x_nose > 0) {
                        if (x_nose < 320)
                            PersonLocation = 2;
                        else
                            PersonLocation = 1;
                        bFound = true;
                    } else if (x_chest > 0) {
                        if (x_chest < 320)
                            PersonLocation = 2;
                        else
                            PersonLocation = 1;
                        bFound = true;
                    } else if (x_lefthip > 0) {
                        if (x_lefthip < 320)
                            PersonLocation = 2;
                        else
                            PersonLocation = 1;
                        bFound = true;

                    } else if (x_righthip > 0) {
                        if (x_righthip < 320)
                            PersonLocation = 2;
                        else
                            PersonLocation = 1;
                        bFound = true;

                    }

                    if (bFound)
                        break;
                }
            }
            if (bFound == false) {
                for (index_search = AnalyzedFrameArray.length - 1; index_search > frame_buffer_index; index_search--) {
                    if (AnalyzedFrameArray[index_search].bFoundPerson) {
                        //check the left or right side
                        float x_nose = AnalyzedFrameArray[index_search].fMatrix[0][1];
                        float x_chest = AnalyzedFrameArray[index_search].fMatrix[1][1];
                        float x_lefthip = AnalyzedFrameArray[index_search].fMatrix[8][1];
                        float x_righthip = AnalyzedFrameArray[index_search].fMatrix[11][1];
                        if (x_nose > 0) {
                            if (x_nose < 320)
                                PersonLocation = 2;
                            else
                                PersonLocation = 1;
                            bFound = true;
                        } else if (x_chest > 0) {
                            if (x_chest < 320)
                                PersonLocation = 2;
                            else
                                PersonLocation = 1;
                            bFound = true;
                        } else if (x_lefthip > 0) {
                            if (x_lefthip < 320)
                                PersonLocation = 2;
                            else
                                PersonLocation = 1;
                            bFound = true;

                        } else if (x_righthip > 0) {
                            if (x_righthip < 320)
                                PersonLocation = 2;
                            else
                                PersonLocation = 1;
                            bFound = true;

                        }

                        if (bFound)
                            break;
                    }
                }
            }
            mLastPersonLocationLeftOrRight = PersonLocation;
            m_bPersonLocationReset = false;
            return PersonLocation;
        } else {
            return mLastPersonLocationLeftOrRight;
        }
    }

    public boolean CheckTurnOneAround() {
        //If the least 12 actions_mode records are 4 or 5, return true, else, return false
        int number_of_checked_frames = 12;
        LinkedList<Integer> list = new LinkedList<>();
        int index = action_buffer_index;
        int lower_bound = max(0, action_buffer_index - number_of_checked_frames + 1);
        for (; index >= lower_bound; index--) {
            list.add(index);
        }
        if (action_buffer_index +1 < number_of_checked_frames) {
            for (index = ActionArray.length - 1; index >= ActionArray.length - number_of_checked_frames + action_buffer_index; index--) {
                list.add(index);
            }
        }

        int[][] pattern = {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}};
        boolean[] bArrayFound = {true, true};
        for( int index_pattern = 0 ; index_pattern < pattern[0].length ; index_pattern++ )
        {
            index = list.poll();
            for (int found_index = 0; found_index < bArrayFound.length; found_index++) {
                if (ActionArray[index] != pattern[found_index][index_pattern])
                    bArrayFound[found_index] = false;
            }
        }

        if (bArrayFound[0] || bArrayFound[1])
            return true;
        else
            return false;
    }


    public boolean CheckTurnTwoAround() {
        //If the least 12 actions_mode records are 4 or 5, return true, else, return false
        int number_of_checked_frames = 25;
        LinkedList<Integer> list = new LinkedList<>();
        int index = action_buffer_index;
        int lower_bound = max(0, action_buffer_index - number_of_checked_frames + 1);
        for (; index >= lower_bound; index--) {
            list.add(index);
        }
        if (action_buffer_index +1 < number_of_checked_frames) {
            for (index = ActionArray.length - 1; index >= ActionArray.length - number_of_checked_frames + action_buffer_index; index--) {
                list.add(index);
            }
        }

        int[][] pattern = {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}};
        boolean[] bArrayFound = {true, true};
        for( int index_pattern = 0 ; index_pattern < pattern[0].length ; index_pattern++ )
        {
            index = list.poll();
            for (int found_index = 0; found_index < bArrayFound.length; found_index++) {
                if (ActionArray[index] != pattern[found_index][index_pattern])
                    bArrayFound[found_index] = false;
            }
        }

        if (bArrayFound[0] || bArrayFound[1])
            return true;
        else
            return false;
    }

    //If the previous N frames contain no person
    public boolean bConsecutiveNoPattern(int number_of_checked_frames)
    {
        LinkedList<Integer> list = new LinkedList<>();
        int index = frame_buffer_index - 1;
        int lower_bound = max(0, frame_buffer_index - number_of_checked_frames);
        for (; index >= lower_bound; index--) {
//            if( AnalyzedFrameArray[index].bAvailable)
                list.add(index);
        }
        if(frame_buffer_index - number_of_checked_frames < 0)
        for (index = AnalyzedFrameArray.length - 1; index >= AnalyzedFrameArray.length-1-(frame_buffer_index - number_of_checked_frames); index--) {
//            if( AnalyzedFrameArray[index].bAvailable)
                list.add(index);
        }

        boolean bMatchPattern = true;
        while( !list.isEmpty())
        {
            index = list.poll();
            if( AnalyzedFrameArray[index].bFoundPerson == true ) {
                bMatchPattern = false;
                break;
            }
        }

        return bMatchPattern;
    }

    //reset the number of turning around to let Zenbo turn around again
    public void BreakStandbyPattern() {
        ActionArray[action_buffer_index] = 7;        //alter the pattern
    }
}
