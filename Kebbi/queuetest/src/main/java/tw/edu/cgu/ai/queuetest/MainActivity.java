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
                mRobot.showWindow(false);
                mRobot.hideWindow(true);     //to diable the overlapping window
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
                    mRobot.startTTS("Zero.Zero.Zero.Zero.");
                    mRobot.motionPlay("666_BA_ArmSCircle", false);
                    mRobot.startTTS("One.One.One.One.");
                    mRobot.motionPlay("666_BA_MoveF3cm", false);
                    mRobot.startTTS("Two.Two.Two.Two.");
                    mRobot.motionPlay("666_BA_RArmCircleL", false);
                    mRobot.startTTS("Three.Three.Three.Three.");
                    mRobot.motionPlay("666_BA_RArmClose", false);
                    mRobot.hideWindow(true);     //to diable the overlapping window
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