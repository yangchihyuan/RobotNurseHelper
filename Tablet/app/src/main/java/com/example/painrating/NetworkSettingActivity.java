package com.example.painrating;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.provider.Contacts;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.net.Socket;

public class NetworkSettingActivity extends AppCompatActivity {

    private Button btnTest, btnSave;
    private EditText editText_Server;
    private EditText editText_Port;

    private String mServerURL;
    private Integer mPortNumber;

    Socket SocketTest;

    protected void UItoVariables(){
        mPortNumber = Integer.parseInt(editText_Port.getText().toString());
        mServerURL = editText_Server.getText().toString();
    }

    protected void RetrieveSharedPreferences(){
        SharedPreferences sharedPref = getSharedPreferences("PainRating_Preference", Context.MODE_PRIVATE);
        String ServerURL = sharedPref.getString("ServerURL", "");
        if (!ServerURL.isEmpty()) {
            editText_Server.setText(ServerURL);
            mServerURL = ServerURL;
        }

        String PortNumber = sharedPref.getString("PortNumber", "");
        if (!PortNumber.isEmpty()) {
            editText_Port.setText(PortNumber);
            mPortNumber = Integer.parseInt(PortNumber);
        }
    }

    protected void SaveSharedPreferences(){
        UItoVariables();

        SharedPreferences sharedPref = getSharedPreferences("PainRating_Preference", Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("ServerURL", mServerURL.toString());
        editor.putString("PortNumber", mPortNumber.toString());
        editor.apply();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_network_setting);

        editText_Port = (EditText) findViewById(R.id.editText_Port);
        editText_Server = (EditText) findViewById(R.id.editText_Server);
        btnSave = (Button) findViewById(R.id.btnSave);
        btnTest = (Button) findViewById(R.id.btnTest);

        UItoVariables();
        RetrieveSharedPreferences();

        btnSave.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                SaveSharedPreferences();
                Toast.makeText(NetworkSettingActivity.this, "Server URL and Port Number saved", Toast.LENGTH_LONG).show();
            }
        });

        btnTest.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                SaveSharedPreferences();

                HandlerThread thread = new HandlerThread("SocketProcess");
                thread.start();
                Handler handler = new Handler(thread.getLooper());
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            SocketTest = new Socket(mServerURL, mPortNumber);
                            if (SocketTest.isConnected()) {
                                System.out.println("Connected");
                                Toast.makeText(NetworkSettingActivity.this, "Connected", Toast.LENGTH_LONG).show();
                            } else {
                            }
                            SocketTest.close();
                        } catch (Exception e) {
                            e.printStackTrace();
                            Toast.makeText(NetworkSettingActivity.this, "Not Connected", Toast.LENGTH_LONG).show();
                        }
                    }
                });
            }
        });

    }
}
