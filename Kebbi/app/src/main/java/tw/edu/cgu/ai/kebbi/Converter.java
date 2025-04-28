package tw.edu.cgu.ai.kebbi;

//import com.asus.robotframework.API.RobotFace;
//import com.asus.robotframework.API.Utility;
import android.graphics.Bitmap;
import android.graphics.Matrix;

public class Converter {
    /*
//    static public RobotFace FaceIndexToRobotFace(int FaceIndex)
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

    static public int PredefinedActionIndexToPlayAction(int PredefinedActionIndex)
    {

        int theAction = Utility.PlayAction.Default_1;
        switch(PredefinedActionIndex) {
            case 0:
                theAction = Utility.PlayAction.Body_twist_1;
                break;
            case 1:
                theAction = Utility.PlayAction.Body_twist_2;
                break;
            case 2:
                theAction = Utility.PlayAction.Dance_2_loop;
                break;
            case 3:
                theAction = Utility.PlayAction.Dance_3_loop;
                break;
            case 4:
                theAction = Utility.PlayAction.Dance_b_1_loop;
                break;
            case 5:
                theAction = Utility.PlayAction.Dance_s_1_loop;
                break;
            case 6:
                theAction = Utility.PlayAction.Default_1;
                break;
            case 7:
                theAction = Utility.PlayAction.Default_2;
                break;
            case 8:
                theAction = Utility.PlayAction.Find_face;
                break;
            case 9:
                theAction = Utility.PlayAction.Head_down_1;
                break;
            case 10:
                theAction = Utility.PlayAction.Head_down_2;
                break;
            case 11:
                theAction = Utility.PlayAction.Head_down_3;
                break;
            case 12:
                theAction = Utility.PlayAction.Head_down_4;
                break;
            case 13:
                theAction = Utility.PlayAction.Head_down_5;
                break;
            case 14:
                theAction = Utility.PlayAction.Head_down_7;
                break;
            case 15:
                theAction = Utility.PlayAction.Head_twist_1_loop;
                break;
            case 16:
                theAction = Utility.PlayAction.Head_up_1;
                break;
            case 17:
                theAction = Utility.PlayAction.Head_up_2;
                break;
            case 18:
                theAction = Utility.PlayAction.Head_up_3;
                break;
            case 19:
                theAction = Utility.PlayAction.Head_up_4;
                break;
            case 20:
                theAction = Utility.PlayAction.Head_up_5;
                break;
            case 21:
                theAction = Utility.PlayAction.Head_up_6;
                break;
            case 22:
                theAction = Utility.PlayAction.Head_up_7;
                break;
            case 23:
                theAction = Utility.PlayAction.Music_1_loop;
                break;
            case 24:
                theAction = Utility.PlayAction.Nod_1;
                break;
            case 25:
                theAction = Utility.PlayAction.Shake_head_1;
                break;
            case 26:
                theAction = Utility.PlayAction.Shake_head_2;
                break;
            case 27:
                theAction = Utility.PlayAction.Shake_head_3;
                break;
            case 28:
                theAction = Utility.PlayAction.Shake_head_4_loop;
                break;
            case 29:
                theAction = Utility.PlayAction.Shake_head_5;
                break;
            case 30:
                theAction = Utility.PlayAction.Shake_head_6;
                break;
            case 32:
                theAction = Utility.PlayAction.Turn_left_1;
                break;
            case 33:
                theAction = Utility.PlayAction.Turn_left_2;
                break;
            case 34:
                theAction = Utility.PlayAction.Turn_left_reverse_1;
                break;
            case 35:
                theAction = Utility.PlayAction.Turn_left_reverse_2;
                break;
            case 36:
                theAction = Utility.PlayAction.Turn_right_1;
                break;
            case 37:
                theAction = Utility.PlayAction.Turn_right_2;
                break;
            case 38:
                theAction = Utility.PlayAction.Turn_right_reverse_1;
                break;
            case 39:
                theAction = Utility.PlayAction.Turn_right_reverse_2;
                break;
        }
        return theAction;
    }
*/
    static public byte [] ShortToByte_Twiddle_Method(short [] input)
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

    static public Bitmap RotateImage180Degree(Bitmap source) {
        Matrix matrix = new Matrix();
        matrix.postScale(-1, -1, source.getWidth() / 2f, source.getHeight() / 2f);
        return Bitmap.createBitmap(source, 0, 0, source.getWidth(), source.getHeight(), matrix, true);
    }
}
