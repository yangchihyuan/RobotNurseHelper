package tw.edu.cgu.ai.kebbi.pressnumber;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;

import tw.edu.cgu.ai.kebbi.pressnumber.R;

import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class MainActivity extends AppCompatActivity {
    private ImageButton btn1, btn2, btn3, btn4, btn5;
    //btnGoPain, btnGoVideo,
    private Button btnNetworkSetting;
    private String mServerURL;
    private Integer mPortNumber;

    Socket SocketToServer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btn1 = findViewById(R.id.btn1);
        btn2 = findViewById(R.id.btn2);
        btn3 = findViewById(R.id.btn3);
        btn4 = findViewById(R.id.btn4);
        btn5 = findViewById(R.id.btn5);
        btnNetworkSetting = findViewById(R.id.btnNetworkSetting);

        btn1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SendMessageToServer(1);
            }
        });

        btn2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SendMessageToServer(2);
            }
        });

        btn3.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SendMessageToServer(3);
            }
        });

        btn4.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SendMessageToServer(4);
            }
        });

        btn5.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SendMessageToServer(5);
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

    protected void SendMessageToServer(int number) {
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
                        buffer2.putInt(number);
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

}
