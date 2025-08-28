package tw.edu.cgu.ai.kebbi;

import android.Manifest;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;

import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.camera.view.PreviewView;

import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Toast;

import android.widget.Button;

import android.content.Intent;

import androidx.core.content.ContextCompat;

public class MainActivity extends Activity implements CameraService.ServiceCallback {

    private static final int PERMISSIONS_REQUEST = 1;

    private static final String PERMISSION_CAMERA = Manifest.permission.CAMERA;
    private static final String PERMISSION_STORAGE = Manifest.permission.WRITE_EXTERNAL_STORAGE;
    private static final String PERMISSION_RECORD_AUDIO = Manifest.permission.RECORD_AUDIO;

    private EditText editText_Server;
    private EditText editText_Port;

    private CameraService cameraService;
    private boolean isBound = false;
    private PreviewView previewView;

    private static final int LAUNCH_PLAYER_REQUEST = 1001;

    private final ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            CameraService.LocalBinder binder = (CameraService.LocalBinder) service;
            cameraService = binder.getService();
            isBound = true;
            startPreview();
            binder.setCallback(MainActivity.this);
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            isBound = false;
        }
    };


    private void showToast(final String text) {
        MainActivity.this.runOnUiThread(
                () -> Toast.makeText(MainActivity.this, text, Toast.LENGTH_SHORT).show()
        );
    }

    //when the activity become visible
    @Override
    protected void onStart() {
        super.onStart();

/*
        View decorView = getWindow().getDecorView();
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
        decorView.setSystemUiVisibility(uiOptions);
 */
    }


    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //Launch service
        Intent intent = new Intent(this, CameraService.class);
        ContextCompat.startForegroundService(this, intent);
        bindService(intent, connection, Context.BIND_AUTO_CREATE);              //connection is a ServiceConnection object.

        setContentView(R.layout.main_activity);

        if(!hasPermission()) {
            //This is a new thread, we don't know when users will finish it.
            requestPermission();
        }

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        editText_Port = findViewById(R.id.editText_Port);
        editText_Server = findViewById(R.id.editText_Server);
        CheckBox checkBox_enable_connection = findViewById(R.id.checkBox_connect);
        Button button_close = findViewById(R.id.button_close);
        // init kiwi sdk
        previewView = findViewById(R.id.camera_preview_surface);

        checkAndStartService();


        //get the default ServerURL
        SharedPreferences sharedPref = getSharedPreferences("RobotNurseHelper_Preference", Context.MODE_PRIVATE);
        String ServerURL = sharedPref.getString("ServerURL", "");
        if( !ServerURL.isEmpty() ){
            editText_Server.setText(ServerURL);
        }

        checkBox_enable_connection.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @RequiresPermission(Manifest.permission.RECORD_AUDIO)
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    //Save the IP address to SharedPreferences
                    SharedPreferences sharedPref = getSharedPreferences("RobotNurseHelper_Preference", Context.MODE_PRIVATE);
                    SharedPreferences.Editor editor = sharedPref.edit();
                    String sServerURL = editText_Server.getText().toString();
                    int iPortNumber = Integer.parseInt(editText_Port.getText().toString());
                    editor.putString("ServerURL", sServerURL);
                    editor.apply();

                    cameraService.startConnection(sServerURL, iPortNumber);
                }
                else {
                    cameraService.stopConnection();
                }
            }
        });

        button_close.setOnClickListener(new View.OnClickListener() {
                                            @Override
                                            public void onClick(View v) {
                                                android.os.Process.killProcess(android.os.Process.myPid());
                                            }
                                        }
        );

    }

    @Override
    public void launchPlayer(Integer dance_type) {
        Intent intent = new Intent();
        ComponentName comp = new ComponentName("com.nuwarobotics.app.nuwaplayer","com.nuwarobotics.app.nuwaplayer.PlayContentEditorActivity");
        intent.setComponent(comp);
        intent.setAction("com.nuwarobotics.app.nuwaplayer.action.PLAY_MBTX");
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        if (dance_type == 1)
        {
            intent.putExtra("PlayId", "Egypt_Dance-v2");
        }
        else if(dance_type == 2)
        {
            intent.putExtra("PlayId", "Dancing_Cowboy-v2");
        }
        else if(dance_type == 3)
        {
            intent.putExtra("PlayId", "Short_Test_Project");
        }
        else if(dance_type == 4)
        {
            intent.putExtra("PlayId", "Health_Video");
        }

        this.startActivityForResult(intent, LAUNCH_PLAYER_REQUEST);
    }

    //not be called. Nuwa's com.nuwarobotics.app.nuwaplayer.PlayContentEditorActivity does not use the setResult()
    //Thus, I have to use onRestart to
    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        // Check if the result is from our specific request
        if (requestCode == LAUNCH_PLAYER_REQUEST) {
            if (resultCode == RESULT_OK ) {
                //notify the server the dance complete
                cameraService.NotifyServer("onActivityResult");
            }
        }
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        cameraService.NotifyServer("onActivityRestart");
    }

    //2025/1/3 This is a call back function, using the same thread as onCreate(). Thus, it is only be called after the onCreated is completed.
    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSIONS_REQUEST: {
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED
                        && grantResults[1] == PackageManager.PERMISSION_GRANTED
                        && grantResults[2] == PackageManager.PERMISSION_GRANTED) {
                } else {
                    requestPermission();
                }
            }
        }
    }

    //When the Activity get focus
    @Override
    protected void onResume() {
        super.onResume();

        /*
        if (checkSelfPermission(PERMISSION_CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.CAMERA}, 100);
        }
        */

    }

    private boolean hasPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return checkSelfPermission(PERMISSION_CAMERA) == PackageManager.PERMISSION_GRANTED &&
                    checkSelfPermission(PERMISSION_STORAGE) == PackageManager.PERMISSION_GRANTED &&
                    checkSelfPermission(PERMISSION_RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED;
        } else {
            return true;
        }
    }

    private void requestPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (shouldShowRequestPermissionRationale(PERMISSION_CAMERA) ||
                    shouldShowRequestPermissionRationale(PERMISSION_STORAGE) ||
                    shouldShowRequestPermissionRationale(PERMISSION_RECORD_AUDIO)) {
                Toast.makeText(this, "Please grant permissions to execute this program", Toast.LENGTH_LONG).show();
            }
            requestPermissions(new String[]{PERMISSION_CAMERA, PERMISSION_STORAGE, PERMISSION_RECORD_AUDIO}, PERMISSIONS_REQUEST);
        }
    }

    //partially visible, but lost user focus
    @Override
    protected void onPause() {
        super.onPause();
    }

    //when an activity is no longer visible to the user
    @Override
    protected void onStop()
    {
        super.onStop();
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        if (isBound) {
            unbindService(connection);
            isBound = false;
        }
    }

    private void startPreview() {
        if (isBound && cameraService != null) {
            cameraService.startCameraPreview(previewView);
        }
    }


    private void checkAndStartService() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                == PackageManager.PERMISSION_GRANTED) {
            startCameraService();
        }
    }

    private void startCameraService() {
        Intent intent = new Intent(this, CameraService.class);
        // Note: For Android 8.0 (Oreo) and above, use startForegroundService()
        // to start a foreground service.
        ContextCompat.startForegroundService(this, intent);
    }
}
