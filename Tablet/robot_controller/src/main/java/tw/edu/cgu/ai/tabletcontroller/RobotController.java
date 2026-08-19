package tw.edu.cgu.ai.tabletcontroller;

import android.content.Context;
import android.content.SharedPreferences;

import com.google.protobuf.Timestamp;

import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.time.Instant;

import tw.edu.cgu.ai.RobotCommandOuterClass.RobotToServerMessage;

public class RobotController {

    private String serverIP;
    private int serverPort;

    public RobotController(Context context) {

        SharedPreferences sharedPref =
                context.getSharedPreferences(
                        "TabletController_Preference",
                        Context.MODE_PRIVATE);

        serverIP =
                sharedPref.getString("ServerURL", "");

        String port =
                sharedPref.getString("PortNumber", "8898");

        serverPort = Integer.parseInt(port);

    }

    //-------------------------------------------------------
    // Send Command
    //-------------------------------------------------------

    public boolean sendCommand(String command) {

        Socket socket = null;

        try {

            socket = new Socket(serverIP, serverPort);

            OutputStream os = socket.getOutputStream();

            os.write("BeginOfADataFrame".getBytes());

            Instant instant = Instant.now();

            Timestamp time =
                    Timestamp.newBuilder()
                            .setSeconds(
                                    instant.getEpochSecond())
                            .setNanos(
                                    instant.getNano())
                            .build();

            RobotToServerMessage message =
                    RobotToServerMessage.newBuilder()
                            .setTabletcommand(command)
                            .setEventTime(time)
                            .build();

            byte[] data = message.toByteArray();

            ByteBuffer buffer =
                    ByteBuffer.allocate(4);

            buffer.order(ByteOrder.LITTLE_ENDIAN);

            buffer.putInt(data.length);

            os.write(buffer.array());

            os.write(data);

            os.write("EndOfADataFrame".getBytes());

            os.flush();

            socket.close();

            return true;

        }
        catch (Exception e){

            e.printStackTrace();

            return false;

        }

    }

}