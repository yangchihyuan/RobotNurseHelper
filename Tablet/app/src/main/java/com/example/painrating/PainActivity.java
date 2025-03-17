package com.example.painrating;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.RectF;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.Toast;
import android.content.Intent;

import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;

public class PainActivity extends AppCompatActivity {

    // 這兩個常數代表圖的原始像素尺寸 (寬、高)
    private static final float ORIG_IMG_WIDTH = 2346f;
    private static final float ORIG_IMG_HEIGHT = 840f;

    // 每個臉對應的「在原圖中的座標範圍」
    private final List<RectF> faceBounds = new ArrayList<>();

    // 把 ImageView 宣告為成員 (class-level field)，避免 “needs to be declared final” 問題
    private ImageView imgPainScale;

    private String mServerURL;
    private Integer mPortNumber;

    Socket SocketToServer;
    int gfaceIndex;

    protected void RetrieveSharedPreferences(){
        SharedPreferences sharedPref = getSharedPreferences("PainRating_Preference", Context.MODE_PRIVATE);
        String ServerURL = sharedPref.getString("ServerURL", "");
        if (!ServerURL.isEmpty()) {
            mServerURL = ServerURL;
        }

        String PortNumber = sharedPref.getString("PortNumber", "");
        if (!PortNumber.isEmpty()) {
            mPortNumber = Integer.parseInt(PortNumber);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_pain);
        // ↑ 請在此佈局中放一個 ImageView (id=@+id/imgPainScale)，顯示整張量表圖

        // 1) 設定臉譜區域 (假設 0,2,4,6,8,10 六個臉)
        //   以下範例座標是 (left, top, right, bottom)；請依實際圖來調整
        faceBounds.add(new RectF( 0, 100, 391, 700));  // 0分
        faceBounds.add(new RectF(391, 100, 782, 700));  // 2分
        faceBounds.add(new RectF(782, 100, 1173, 700));  // 4分
        faceBounds.add(new RectF(1173, 100, 1564, 700));  // 6分
        faceBounds.add(new RectF(1564, 100, 1955, 700));  // 8分
        faceBounds.add(new RectF(1955, 100, 2346, 700));  // 10分

        // 2) 連結 ImageView
        imgPainScale = findViewById(R.id.imgPainScale);

        // 3) 設定觸控監聽器 (僅示範 ACTION_DOWN)
        imgPainScale.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    // (A) 取得使用者點擊的 x、y (相對 imgPainScale 的左上角)
                    float clickX = event.getX();
                    float clickY = event.getY();

                    // (B) 取得 ImageView 實際顯示寬、高
                    float displayedWidth = imgPainScale.getWidth();
                    float displayedHeight = imgPainScale.getHeight();

                    // (C) 把點擊位置換算回「原圖」的座標
                    float ratioX = clickX / displayedWidth;
                    float ratioY = clickY / displayedHeight;
                    float originalX = ratioX * ORIG_IMG_WIDTH;
                    float originalY = ratioY * ORIG_IMG_HEIGHT;

                    // (D) 判斷落在哪個臉區域
                    int faceIndex = getFaceIndex(originalX, originalY);
                    if (faceIndex == -1) {
                        Toast.makeText(PainActivity.this,
                                "未點中任何臉", Toast.LENGTH_SHORT).show();
                    } else {
                        // 顯示分數
                        showFaceToast(faceIndex);
                        gfaceIndex = faceIndex;
                    }

                    //There are two types of touch events: ACTION_DOWN and ACTION_UP.
                    //Create a socket, send a message to the server and close the socket.
                    HandlerThread thread = new HandlerThread("SocketProcess");
                    thread.start();
                    Handler handler = new Handler(thread.getLooper());
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                RetrieveSharedPreferences();
                                SocketToServer = new Socket(mServerURL, mPortNumber);
                                if (SocketToServer.isConnected()) {
                                    OutputStream os = SocketToServer.getOutputStream();
                                    os.write("Begin:".getBytes());
                                    Long message_length = (long) (4);   //value
                                    ByteBuffer buffer = ByteBuffer.allocate(8);
                                    buffer.order(ByteOrder.LITTLE_ENDIAN); // Ubuntu byte order
                                    buffer.putLong(message_length);
                                    byte[] byteArray = buffer.array();
                                    os.write(byteArray);

                                    ByteBuffer buffer2 = ByteBuffer.allocate(4);
                                    buffer2.order(ByteOrder.LITTLE_ENDIAN); // Ubuntu byte order
                                    buffer2.putInt(gfaceIndex);
                                    byte[] byteArray2 = buffer2.array();
                                    os.write(byteArray2);

//                                os.write(gfaceIndex);   //Here is the bug, only 1 byte is sent. Need to send 4 bytes. Maybe there is an implicit convertion.
                                    os.write("EndOfAFrame".getBytes());
                                } else {
                                }
                                SocketToServer.close();
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                    });




                }


                return true; // return true 表示消費該事件
            }
        });


    }

    /**
     * 尋找 (x, y) 是否落在 faceBounds 裡任何一個 RectF
     * @return 回傳臉的索引(0~5)，或 -1 代表找不到
     */
    private int getFaceIndex(float x, float y) {
        for (int i = 0; i < faceBounds.size(); i++) {
            RectF rect = faceBounds.get(i);
            if (rect.contains(x, y)) {
                return i;
            }
        }
        return -1;
    }

    /**
     * 依臉的索引顯示對應分數的 Toast (示範)
     */
    private void showFaceToast(int index) {
        switch (index) {
            case 0:
                // 選擇 0分
                startFaceResultActivity(0);
                break;
            case 1:
                startFaceResultActivity(2);
                break;
            case 2:
                startFaceResultActivity(4);
                break;
            case 3:
                startFaceResultActivity(6);
                break;
            case 4:
                startFaceResultActivity(8);
                break;
            case 5:
                startFaceResultActivity(10);
                break;
        }
    }
    private void startFaceResultActivity(int score) {
        // 用 Intent 帶分數到 FaceResultActivity
        Intent intent = new Intent(PainActivity.this, FaceResultActivity.class);
        intent.putExtra("score", score);
        startActivity(intent);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            SocketToServer.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
