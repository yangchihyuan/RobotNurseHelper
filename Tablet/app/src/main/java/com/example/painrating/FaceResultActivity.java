package com.example.painrating;

import androidx.appcompat.app.AppCompatActivity;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.ViewTreeObserver;
import android.widget.ImageView;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;


public class FaceResultActivity extends AppCompatActivity {

    private ImageView imgResultFace;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_face_result);

        imgResultFace = findViewById(R.id.imgResultFace);

        // 1. 接收上一頁帶來的分數
        int score = getIntent().getIntExtra("score", -1);
        switch (score) {
            case 0:  imgResultFace.setImageResource(R.drawable.face0);  break;
            case 2:  imgResultFace.setImageResource(R.drawable.face2);  break;
            case 4:  imgResultFace.setImageResource(R.drawable.face4);  break;
            case 6:  imgResultFace.setImageResource(R.drawable.face6);  break;
            case 8:  imgResultFace.setImageResource(R.drawable.face8);  break;
            case 10: imgResultFace.setImageResource(R.drawable.face10); break;
            default: break;
        }

        // 2. 先把圖片縮放到 0
        imgResultFace.setScaleX(0f);
        imgResultFace.setScaleY(0f);

        // 3. 等佈局完成後，計算 pivot
        imgResultFace.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        imgResultFace.getViewTreeObserver().removeOnGlobalLayoutListener(this);

                        // A) 螢幕中心
                        DisplayMetrics dm = getResources().getDisplayMetrics();
                        float screenCenterX = dm.widthPixels / 2f;
                        float screenCenterY = dm.heightPixels / 2f;

                        // B) 取得圖片左上角(絕對座標)
                        int[] loc = new int[2];
                        imgResultFace.getLocationOnScreen(loc);
                        float imageLeft = loc[0];
                        float imageTop  = loc[1];

                        // C) pivot = (螢幕中心) - (圖片左上角)
                        float pivotX = screenCenterX - imageLeft;
                        float pivotY = screenCenterY - imageTop;

                        // 設定縮放中心
                        imgResultFace.setPivotX(pivotX);
                        imgResultFace.setPivotY(pivotY);

                        // 4. 執行放大動畫
                        doScaleAnimation();
                    }
                }
        );
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {

                Intent intent = new Intent(FaceResultActivity.this, MainActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
                startActivity(intent);
            }
        }, 30000);
    }


    private void doScaleAnimation() {
        ObjectAnimator animX = ObjectAnimator.ofFloat(imgResultFace, "scaleX", 0f, 1f);
        ObjectAnimator animY = ObjectAnimator.ofFloat(imgResultFace, "scaleY", 0f, 1f);

        AnimatorSet set = new AnimatorSet();
        set.playTogether(animX, animY);
        set.setDuration(1000);
        set.start();
    }
}