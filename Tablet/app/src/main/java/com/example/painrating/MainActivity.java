package com.example.painrating;

import androidx.appcompat.app.AppCompatActivity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;

import java.net.Socket;

public class MainActivity extends AppCompatActivity {

    private ImageButton btnGoPain, btnGoVideo;
    private Button btnNetworkSetting;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnGoPain = findViewById(R.id.btnGoPain);
        btnGoVideo = findViewById(R.id.btnGoVideo);
        btnNetworkSetting = findViewById(R.id.btnNetworkSetting);

        // 1. 點擊 -> PainActivity
        btnGoPain.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(MainActivity.this, PainActivity.class);
                startActivity(intent);
            }
        });

        // 2. 點擊 -> VideoActivity
        btnGoVideo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(MainActivity.this, VideoActivity.class);
                startActivity(intent);
            }
        });

        btnNetworkSetting.setOnClickListener( new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(MainActivity.this, NetworkSettingActivity.class);
                startActivity(intent);
            }
        });
    }
}
