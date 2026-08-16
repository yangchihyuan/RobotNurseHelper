package tw.edu.cgu.ai.tabletcontroller;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.view.View;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {

    //----------------------------------------------------
    // UI
    //----------------------------------------------------

    private View cardStart;
    private View cardPause;
    private View cardRepeat;
    private View cardStop;

    private Button btnEnglish;
    private Button btnMandarin;
    private Button btnNetworkSetting;

    private TextView txtStatus;
    private TextView txtRobotState;
    private TextView txtLessonProgress;
    private TextView txtSessionTimer;

    private ProgressBar progressSession;

    private RecyclerView recyclerConversation;

    //----------------------------------------------------
    // Conversation
    //----------------------------------------------------

    private ConversationAdapter adapter;

    private ArrayList<ConversationMessage> messages;

    //----------------------------------------------------
    // Managers
    //----------------------------------------------------

    private RobotController robotController;

    private CataractSession session;

    //----------------------------------------------------
    // Session State
    //----------------------------------------------------

    private boolean sessionRunning = false;

    private boolean isEnglish = true;

    private long sessionStartTime = 0;

    private Handler timerHandler = new Handler();

    //----------------------------------------------------
    // Activity
    //----------------------------------------------------

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        //------------------------------------------------
        // Managers
        //------------------------------------------------

        robotController = new RobotController(this);

        session = new CataractSession();

        //------------------------------------------------
        // UI
        //------------------------------------------------

        cardStart = findViewById(R.id.cardStart);
        cardPause = findViewById(R.id.cardPause);
        cardRepeat = findViewById(R.id.cardRepeat);
        cardStop = findViewById(R.id.cardStop);

        btnEnglish = findViewById(R.id.radioButton_English);
        btnMandarin = findViewById(R.id.radioButton_Mandarin);

        btnNetworkSetting =
                findViewById(R.id.btnNetworkSetting);

        txtStatus =
                findViewById(R.id.txtStatus);

        txtRobotState =
                findViewById(R.id.txtRobotState);

        txtLessonProgress =
                findViewById(R.id.txtLessonProgress);

        txtSessionTimer =
                findViewById(R.id.txtSessionTimer);

        progressSession =
                findViewById(R.id.progressSession);

        recyclerConversation = findViewById(R.id.recyclerConversation);

        // Initialize data structures regardless of layout configuration
        messages = new ArrayList<>();
        adapter = new ConversationAdapter(messages);

        // Defensive check: ensure RecyclerView is present before configuring it
        if (recyclerConversation == null) {
            android.util.Log.e("MainActivity", "RecyclerView with id 'recyclerConversation' not found in layout");
            // UI will function without conversation list; adapter remains ready for future use
        } else {
            //------------------------------------------------
            // RecyclerView
            //------------------------------------------------
            recyclerConversation.setLayoutManager(new LinearLayoutManager(this));
            recyclerConversation.setAdapter(adapter);
        }

        //------------------------------------------------
        // Initial UI
        //------------------------------------------------

        txtStatus.setText("🟢 Connected");

        txtRobotState.setText("🟢 Connected");

        txtLessonProgress.setText("Lesson 0 / 8");

        txtSessionTimer.setText("00:00");

        progressSession.setProgress(0);

        addRobotMessage("Welcome to Cataract Education Assistant.");

        addRobotMessage("Press Start Session to begin.");
                //------------------------------------------------
        // Start Session
        //------------------------------------------------

        cardStart.setOnClickListener(v -> {

            if (sessionRunning)
                return;

            sessionRunning = true;

            sessionStartTime = System.currentTimeMillis();

            session.restartSession();

            progressSession.setProgress(1);

            txtLessonProgress.setText("Lesson 1 / 8");

            txtRobotState.setText("🟢 Session Running");

            addRobotMessage("Session started.");

            addRobotMessage(session.getCurrentLesson());

            startSessionTimer();

            robotController.sendCommand("StartSession");

        });

        //------------------------------------------------
        // Pause
        //------------------------------------------------

        cardPause.setOnClickListener(v -> {

            if (!sessionRunning)
                return;

            txtRobotState.setText("🟡 Session Paused");

            addRobotMessage("Session paused.");

            robotController.sendCommand("Pause");

        });

        //------------------------------------------------
        // Repeat
        //------------------------------------------------

        cardRepeat.setOnClickListener(v -> {

            if (!sessionRunning)
                return;

            addRobotMessage(session.getCurrentLesson());

            robotController.sendCommand("Repeat");

        });

        //------------------------------------------------
        // Stop
        //------------------------------------------------

        cardStop.setOnClickListener(v -> {

            if (!sessionRunning)
                return;

            sessionRunning = false;

            txtRobotState.setText("🔴 Session Stopped");

            txtLessonProgress.setText("Session Complete");

            progressSession.setProgress(8);

            addRobotMessage("Session finished.");

            timerHandler.removeCallbacksAndMessages(null);

            robotController.sendCommand("Stop");

        });
                //------------------------------------------------
        // English
        //------------------------------------------------

        btnEnglish.setOnClickListener(v -> {

            isEnglish = true;

            addRobotMessage("Language changed to English.");

            robotController.sendCommand("English");

        });

        //------------------------------------------------
        // Mandarin
        //------------------------------------------------

        btnMandarin.setOnClickListener(v -> {

            isEnglish = false;

            addRobotMessage("Language changed to Mandarin.");

            robotController.sendCommand("Mandarin");

        });

        //------------------------------------------------
        // Network Setting
        //------------------------------------------------

        btnNetworkSetting.setOnClickListener(v -> {

            Intent intent = new Intent(
                    MainActivity.this,
                    NetworkSettingActivity.class);

            startActivity(intent);

        });

    }   // End of onCreate()

    //------------------------------------------------
    // Conversation Helpers
    //------------------------------------------------

    private void addRobotMessage(String message) {

        messages.add(new ConversationMessage(message, true));

        if (adapter != null) {
            adapter.notifyItemInserted(messages.size() - 1);
        }
        if (recyclerConversation != null) {
            recyclerConversation.smoothScrollToPosition(messages.size() - 1);
        }
    }

    private void addPatientMessage(String message) {

        messages.add(new ConversationMessage(message, false));

        if (adapter != null) {
            adapter.notifyItemInserted(messages.size() - 1);
        }
        if (recyclerConversation != null) {
            recyclerConversation.smoothScrollToPosition(messages.size() - 1);
        }
    }

    //------------------------------------------------
    // Session Timer
    //------------------------------------------------

    private void startSessionTimer() {

        timerHandler.post(new Runnable() {

            @Override
            public void run() {

                if (!sessionRunning)
                    return;

                long elapsed =
                        System.currentTimeMillis() - sessionStartTime;

                int seconds = (int) (elapsed / 1000);

                int minutes = seconds / 60;

                seconds = seconds % 60;

                txtSessionTimer.setText(
                        String.format("%02d:%02d", minutes, seconds));

                timerHandler.postDelayed(this, 1000);

            }

        });

    }
        //------------------------------------------------
    // Robot Status
    //------------------------------------------------

    private void setRobotConnected() {

        txtStatus.setText("🟢 Connected");

        txtRobotState.setText("🟢 Connected");

    }

    private void setRobotDisconnected() {

        txtStatus.setText("🔴 Disconnected");

        txtRobotState.setText("🔴 Disconnected");

    }

    //------------------------------------------------
    // Progress
    //------------------------------------------------

    private void updateProgress(int lesson, int totalLessons) {

        progressSession.setMax(totalLessons);

        progressSession.setProgress(lesson);

        txtLessonProgress.setText(
                "Lesson " + lesson + " / " + totalLessons);

    }

    //------------------------------------------------
    // Next Lesson
    //------------------------------------------------

    private void nextLesson() {

        String lesson = session.getCurrentLesson();

        addRobotMessage(lesson);

    }

    //------------------------------------------------
    // Activity Cleanup
    //------------------------------------------------

    @Override
    protected void onDestroy() {

        super.onDestroy();

        timerHandler.removeCallbacksAndMessages(null);

    }

}