package com.example.painrating;

import androidx.appcompat.app.AppCompatActivity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;

public class MainActivity extends AppCompatActivity {

    private ImageButton btnGoPain, btnGoVideo;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnGoPain = findViewById(R.id.btnGoPain);
        btnGoVideo = findViewById(R.id.btnGoVideo);

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
    }
}
