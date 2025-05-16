package tw.edu.cgu.ai.zenbo;

import java.util.ArrayList;
import java.util.List;
import ZenboNurseHelperProtobuf.ServerSend.ReportAndCommand;

public class AnalyzedFrame {
    long timestamp_OnImageAvailable;
    long timestamp_ReceivedFromServer;
    int pitchDegree;
    boolean bNew = false;
    int openpose_cnt;
    List<float[][]> openpose_coordinate;
    float[][] fMatrix;
    boolean bFoundPerson = false;
    boolean bIgnorePerson = false;
    boolean bAvailable = false;
    String[] actions = {"",""};     //8/23/2018 Chih-Yuan: I need to modify this statement later

    public AnalyzedFrame()
    {
        fMatrix = new float[18][3];
        openpose_coordinate = new ArrayList<float[][]>();
    }

    public void ParseServerReturn(ReportAndCommand report)
    {
        //clear old data
        openpose_coordinate.clear();

        if(report.hasTimeStamp())
        {
            timestamp_OnImageAvailable = report.getTimeStamp();
        }
        if( report.hasPitchDegree()) {
            pitchDegree = report.getPitchDegree();
        }

        openpose_cnt = report.getPoseCount();
        for( int i = 0 ; i< openpose_cnt ; i++) {
            float[][] coord = new float[18][3];
            ReportAndCommand.OpenPosePose pose = report.getPose(i);
            for( int j = 0; j<18 ; j++) {
                ReportAndCommand.OpenPosePose.OpenPoseCoordinate coordinate = pose.getCoord(j);
                coord[j][0] = (float) coordinate.getX();
                coord[j][1] = (float) coordinate.getY();
                coord[j][2] = (float) coordinate.getValid();
//                if(coordinate.getValid())
//                    coord[j][2] = 1.0f;
//                else
//                    coord[j][2] = 0f;

            }
            openpose_coordinate.add(coord);
        }
        bFoundPerson = openpose_cnt > 0;

        if(openpose_cnt > 1) {
            float dmax = 0;     //the distance of point 0 and 1
            int max_index = 0;
            for( int i = 0 ; i< openpose_cnt ; i++ )
            {
                fMatrix = openpose_coordinate.get(i);
                if( fMatrix[0][2] > 0 && fMatrix[1][2] > 0)
                {
                    float dx = ( fMatrix[0][0] - fMatrix[1][0] );
                    float dy = ( fMatrix[0][1] - fMatrix[1][1] );
                    float dist_square = dx * dx + dy * dy;
                    if( dist_square > dmax)
                    {
                        dmax = dist_square;
                        max_index = i;
                    }
                }
            }
            fMatrix = openpose_coordinate.get(max_index);
        }
        else if( openpose_cnt == 1)
            fMatrix = openpose_coordinate.get(0);
        else
            fMatrix = new float[18][3];

        //Something wrong here, my probability is always 1.0 or 0.0.
        float threshold = 0.4f;
        boolean bExceed = false;
        for(int i=0; i<18; i++)
        {
            if( fMatrix[i][2] > threshold) {
                bExceed = true;
                break;
            }
        }
        if( bExceed)
            bIgnorePerson = false;
        else
            bIgnorePerson =true;

    }
}
