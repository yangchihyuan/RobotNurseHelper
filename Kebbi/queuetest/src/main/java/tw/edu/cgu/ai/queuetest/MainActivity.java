package tw.edu.cgu.ai.queuetest;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.nuwarobotics.service.IClientId;
import com.nuwarobotics.service.agent.NuwaRobotAPI;
import com.nuwarobotics.service.agent.RobotEventListener;
import com.nuwarobotics.service.agent.VoiceEventListener;

public class MainActivity extends AppCompatActivity {

    public NuwaRobotAPI mRobot;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        Button button1 = (Button) findViewById(R.id.button);
        Button button2 = (Button) findViewById(R.id.button2);
        Button button3 = (Button) findViewById(R.id.button3);

        // init kiwi sdk
        String your_app_package_name = getPackageName ();
        IClientId id = new IClientId(your_app_package_name);
        mRobot = new NuwaRobotAPI (this, id);
        mRobot.hideWindow(false);
        mRobot.controlAlwaysWakeup(true);
        mRobot.hideFace();

        mRobot.registerRobotEventListener(new RobotEventListener() {
            @Override
            public void onWikiServiceStart () {
                Log.d("KEBBI", "onWikiServiceStart");
                mRobot.startTTS("Hello, I'm Kebbi.");
            }

            @Override
            public void onWikiServiceStop () {

            }

            @Override
            public void onWikiServiceCrash () {

            }

            @Override
            public void onWikiServiceRecovery () {

            }

            @Override
            public void onStartOfMotionPlay (String s) {

            }

            @Override
            public void onPauseOfMotionPlay (String s) {

            }

            @Override
            public void onStopOfMotionPlay (String s) {

            }

            @Override
            public void onCompleteOfMotionPlay (String s) {

            }

            @Override
            public void onPlayBackOfMotionPlay (String s) {

            }

            @Override
            public void onErrorOfMotionPlay (int i) {

            }

            @Override
            public void onPrepareMotion (boolean b, String s, float v) {

            }

            @Override
            public void onCameraOfMotionPlay (String s) {

            }

            @Override
            public void onGetCameraPose (float v, float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8, float v9, float v10, float v11) {

            }

            @Override
            public void onTouchEvent (int i, int i1) {

            }

            @Override
            public void onPIREvent (int i) {

            }

            @Override
            public void onTap (int i) {

            }

            @Override
            public void onLongPress (int i) {

            }

            @Override
            public void onWindowSurfaceReady () {

            }

            @Override
            public void onWindowSurfaceDestroy () {

            }

            @Override
            public void onTouchEyes (int i, int i1) {

            }

            @Override
            public void onRawTouch (int i, int i1, int i2) {

            }

            @Override
            public void onFaceSpeaker (float v) {

            }

            @Override
            public void onActionEvent (int i, int i1) {

            }

            @Override
            public void onDropSensorEvent (int i) {

            }

            @Override
            public void onMotorErrorEvent (int i, int i1) {

            }
        });


        mRobot.registerVoiceEventListener (new VoiceEventListener () {
            @Override
            public void onWakeup (boolean b, String s, float v) {

            }

            @Override
            public void onTTSComplete (boolean b) {

            }

            @Override
            public void onSpeechRecognizeComplete (boolean b, ResultType resultType, String s) {

            }

            @Override
            public void onSpeech2TextComplete (boolean b, String s) {

            }

            @Override
            public void onMixUnderstandComplete (boolean b, ResultType resultType, String s) {

            }

            @Override
            public void onSpeechState (VoiceEventListener.ListenType
                                               listenType, VoiceEventListener.SpeechState speechState) {

            }

            @Override
            public void onSpeakState (VoiceEventListener.SpeakType
                                              speakType, VoiceEventListener.SpeakState speakState) {

            }

            @Override
            public void onGrammarState (boolean b, String s) {

            }

            @Override
            public void onListenVolumeChanged (VoiceEventListener.ListenType listenType, int i) {

            }

            @Override
            public void onHotwordChange (VoiceEventListener.HotwordState
                                                 hotwordState, VoiceEventListener.HotwordType hotwordType, String s) {

            }
        });

        button1.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    String[] motionArray = {"666_TA_DictateL", "666_DA_Full", "666_EM_Mad02", "666_BA_Nodhead",
                            "666_SP_Swim02", "666_PE_RotateA", "666_SP_Karate", "666_RE_Cheer", "666_SP_Climb", "666_DA_Hit",
                            "666_TA_DictateR", "666_SP_Bowling", "666_SP_Walk", "666_SA_Find", "666_BA_TurnHead", "666_SA_Toothache",
                            "666_SA_Sick","666_SA_Shocked","666_SP_Dumbbell","666_SA_Discover","666_RE_Thanks","666_PE_Changing",
                            "666_SP_HorizontalBar","666_WO_Traffic","666_RE_HiR","666_RE_HiL","666_DA_Brushteeth","666_RE_Encourage",
                            "666_RE_Request","666_PE_Brewing","666_RE_Change","666_PE_Phubbing","666_RE_Baoquan","666_SP_Cheer",
                            "666_RE_Ask","666_PE_Triangel","666_PE_Sorcery","666_PE_Sneak","666_PE_Singing","666_LE_Yoyo","666_SP_Throw",
                            "666_SP_RaceWalk","666_PE_ShakeFart","666_PE_RotateC","666_PE_RotateB","666_EM_Blush","666_PE_Puff",
                            "666_PE_PlayCello","666_PE_Pikachu"};
                    Log.d("Debug", "Receive an action command");
                    mRobot.motionPlay(motionArray[0], true);

                }
            }
        );

        button2.setOnClickListener(new View.OnClickListener() {
                                       @Override
                                       public void onClick(View v) {

                                       }
                                   }
        );

        button3.setOnClickListener(new View.OnClickListener() {
                                       @Override
                                       public void onClick(View v) {

                                       }
                                   }
        );

    }


}