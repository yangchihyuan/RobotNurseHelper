/*  This file has been created by Nataniel Ruiz affiliated with Wall Lab
 *  at the Georgia Institute of Technology School of Interactive Computing
 */

package tw.edu.cgu.ai.zenbo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;


public class KeyPointView extends View {
    //private final Paint Paint_openpose_other_connection, Paint_openpose_first_landmark, Paint_openpose_other_landmark, Paint_Yolo_person, Paint_Yolo_person_text, Paint_Yolo_tvmonitor, Paint_Yolo_tvmonitor_text, Paint_openpose_first_connection;
    private final Paint Paint_Tracker_green, Paint_Tracker_red, Paint_openpose_other_connection, Paint_openpose_first_landmark, Paint_openpose_other_landmark, Paint_openpose_first_connection;
    private AnalyzedFrame analyzedFrame;

    public KeyPointView(final Context context, final AttributeSet set) {
        super(context, set);

        Paint_openpose_first_landmark = new Paint();
        Paint_openpose_first_landmark.setColor(0xff00ff00);  //Green

        Paint_openpose_first_connection = new Paint();
        Paint_openpose_first_connection.setColor(0xff00ff00); //Green

        Paint_openpose_other_landmark = new Paint();
        Paint_openpose_other_landmark.setColor(0xffff0000);     //Red

        Paint_openpose_other_connection = new Paint();
        Paint_openpose_other_connection.setColor(0xffff0000);    //Red

        Paint_Tracker_green = new Paint();
        Paint_Tracker_green.setColor(Color.GREEN);
        Paint_Tracker_green.setStyle(Paint.Style.STROKE);
        Paint_Tracker_green.setStrokeWidth(3.0f);

        Paint_Tracker_red = new Paint();
        Paint_Tracker_red.setColor(Color.RED);
        Paint_Tracker_red.setStyle(Paint.Style.STROKE);
        Paint_Tracker_red.setStrokeWidth(3.0f);
    }

    public void setResults( AnalyzedFrame frame)
    {
        analyzedFrame = frame;
        postInvalidate();       //This function will lead to onDraw
    }

    @Override
    public void onDraw(final Canvas canvas) {
        if( analyzedFrame == null)
            return;

        //draw openpose skeleton
        for( int idx_openpose = 0 ; idx_openpose < analyzedFrame.openpose_cnt ;idx_openpose++)
        {
            Paint Paint_landmark, Paint_connection;
            if( idx_openpose == 0 ) {
                Paint_landmark = Paint_openpose_first_landmark;
                Paint_connection = Paint_openpose_first_connection;
            }
            else {
                Paint_landmark = Paint_openpose_other_connection;
                Paint_connection = Paint_openpose_other_landmark;
            }

            float [][] o_results = analyzedFrame.openpose_coordinate.get(idx_openpose);
            for (int i=0; i<18; ++i) {
                if( o_results[i][2] > 0 )
                {
                    float x = o_results[i][0];
                    float y = o_results[i][1];
                    canvas.drawCircle(x, y, 4, Paint_landmark);
                }
            }

            // Draw skeletal displacement :)
            int[] src = {0,  0,  0, 1, 1, 1,  1, 2, 3, 5, 6, 8,  9, 11, 12, 14, 15};
            int[] dst = {1, 14, 15, 2, 5, 8, 11, 3, 4, 6, 7, 9, 10, 12, 13, 16, 17};

            for(int j = 0; j < 17; j++)
                if(o_results[src[j]][2] > 0 && o_results[dst[j]][2] > 0)
                    canvas.drawLine(o_results[src[j]][0], o_results[src[j]][1],
                                    o_results[dst[j]][0], o_results[dst[j]][1], Paint_connection);

        }

        //Draw person
        /*
        for( int idx_yolo = 0 ; idx_yolo < analyzedFrame.yolo_cnt_person ;idx_yolo++)
        {
            int[] yolo_results = analyzedFrame.yolo_coordinate_person.get(idx_yolo);

            int left = yolo_results[0];
            int right = yolo_results[1];
            int top = yolo_results[2];
            int bottom = yolo_results[3];

            canvas.drawRect(left, top, right, bottom, Paint_Yolo_person);
            canvas.drawText("person", left, top, Paint_Yolo_person_text);
        }
        */
        //Draw TV
        /*
        Log.d("KeyPointView",Integer.toString(analyzedFrame.yolo_cnt_tvmonitor));
        for( int idx_yolo = 0 ; idx_yolo < analyzedFrame.yolo_cnt_tvmonitor ;idx_yolo++)
        {
            //Draw bounding boxes for yolo
            int[] yolo_results = analyzedFrame.yolo_coordinate_tvmonitor.get(idx_yolo);

            int left = yolo_results[0];
            int right = yolo_results[1];
            int top = yolo_results[2];
            int bottom = yolo_results[3];

            canvas.drawRect(left, top, right, bottom, Paint_Yolo_tvmonitor);
            canvas.drawText("TVmonitor", left, top, Paint_Yolo_tvmonitor_text);
        }
        */

        //Draw tracker roi
/*        if( analyzedFrame.tracker_roi_x != -1)
        {
            int left = analyzedFrame.tracker_roi_x;
            int right = analyzedFrame.tracker_roi_x + analyzedFrame.tracker_roi_width;
            int top = analyzedFrame.tracker_roi_y;
            int bottom = analyzedFrame.tracker_roi_y + analyzedFrame.tracker_roi_height;
            if( analyzedFrame.roi_rectangle_color == 1)
                canvas.drawRect(left, top, right, bottom, Paint_Tracker_green);
            else if( analyzedFrame.roi_rectangle_color == 2)
                canvas.drawRect(left, top, right, bottom, Paint_Tracker_red);
        }

 */
    }
}
