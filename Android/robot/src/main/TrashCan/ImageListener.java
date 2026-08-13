/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

/*  This file has been modified by Nataniel Ruiz affiliated with Wall Lab
 *  at the Georgia Institute of Technology School of Interactive Computing
 */

package tw.edu.cgu.ai.kebbi;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.media.Image;
import android.media.Image.Plane;
import android.media.ImageReader;
import android.media.ImageReader.OnImageAvailableListener;

import com.google.protobuf.ByteString;
import com.google.protobuf.Timestamp;

import RobotCommandProtobuf.RobotCommandOuterClass;
import tw.edu.cgu.ai.kebbi.env.ImageUtils;
import tw.edu.cgu.ai.kebbi.env.Logger;

import java.io.ByteArrayOutputStream;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.time.Instant;

class ImageListener implements OnImageAvailableListener {

    static {
        System.loadLibrary("native-lib");
    }

    private static final Logger LOGGER = new Logger();

    //Todo: why do I have the previewWidth and previewHeight here?
    private int previewWidth = 640;
    private int previewHeight = 480;
    //Zenbo supports 1920x1080
//    private int previewWidth = 1920;        //I can change here to get high-resolution images, but very slow
//    private int previewHeight = 1080;
    //Emulator only supports up to 1280x960
//    private int previewWidth = 1280;        //I can change here to get high-resolution images, but very slow
//    private int previewHeight = 960;
    private byte[][] yuvBytes;
    private int[] argbBytes = null;
    private Bitmap Bitmaptemp = null;
    private Bitmap argbFrameBitmap = null;
    private InputView inputView;
    public SocketManager socketManager;

    private Converter converter;

    public void initialize(SocketManager socketManager, InputView inputView) {
        argbFrameBitmap = Bitmap.createBitmap(previewWidth, previewHeight, Config.ARGB_8888);
        Bitmaptemp = Bitmap.createBitmap(previewWidth, previewHeight, Config.ARGB_8888);
        argbBytes = new int[previewWidth * previewHeight];
        this.socketManager = socketManager;
        this.inputView = inputView;
    }

    @Override
    public void onImageAvailable(final ImageReader reader) {
        final Image image = reader.acquireLatestImage();

        if (image == null)
            return; //such a case happens.

        final long timestamp_image = System.currentTimeMillis();

        //2025/8/19 I want to use the new code, but I don't know why it does not work.
        /*
        Instant instant = Instant.now();

        Timestamp time = Timestamp.newBuilder()
                .setSeconds(instant.getEpochSecond())
                .setNanos(instant.getNano())
                .build();
        */
        final Plane[] planes = image.getPlanes();

        yuvBytes = new byte[planes.length][];
        for (int i = 0; i < planes.length; ++i) {
            yuvBytes[i] = new byte[planes[i].getBuffer().capacity()];
            planes[i].getBuffer().get(yuvBytes[i]);
        }

        try {
            final int yRowStride = planes[0].getRowStride();
            final int uvRowStride = planes[1].getRowStride();
            final int uvPixelStride = planes[1].getPixelStride();

            //2024/6/24 Chih-Yuan Yang: Exception occurs in this statement, why?
            ImageUtils.convertYUV420ToARGB8888(
                    yuvBytes[0],
                    yuvBytes[1],
                    yuvBytes[2],
                    argbBytes,
                    previewWidth,
                    previewHeight,
                    yRowStride,
                    uvRowStride,
                    uvPixelStride,
                    false);

            image.close();
        } catch (final Exception e) {
            if (image != null) {
                image.close();
            }
            LOGGER.e(e, "Exception!");
            return;
        }
        Bitmaptemp.setPixels(argbBytes, 0, previewWidth, 0, 0, previewWidth, previewHeight);

        //my laptop's webcam generates upside down images to the simulator. Thus, I need to flip the image.
        boolean rotateImage = false;
        if( EmulatorDetector.isEmulator())
            argbFrameBitmap = converter.RotateImage180Degree(Bitmaptemp);
        else
            argbFrameBitmap = Bitmaptemp;

        inputView.setBitmap(argbFrameBitmap);
        inputView.postInvalidate();

        //Prepare buffer
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        argbFrameBitmap.compress(Bitmap.CompressFormat.JPEG, 50, baos);

        String Timestamp = Long.toString(timestamp_image);
        byte[] array_JPEG = baos.toByteArray();
        String JPEG_length = String.format("%05d", array_JPEG.length);

            int message_length = (int) (Timestamp.length() + 1 + 3 + 1 + JPEG_length.length() + 1 + array_JPEG.length);
            int buffer_length = message_length + 17 + 4 + 15;     //"BeginOfADataFrame" and messagelength (4 bytes) and "EndOfADataFrame"
            String PitchDegree;
            /*
            if (mActionRunnable.pitchDegree >= 0)
                PitchDegree = String.format("%03d", mActionRunnable.pitchDegree);
            else
                PitchDegree = String.format("%02d", Math.abs(mActionRunnable.pitchDegree));
            */
            //I no longer need this.
            PitchDegree = String.format("%03d", 0);
            String isDancing = String.format("%03d", socketManager.dancing_status);
            ByteBuffer buffer = ByteBuffer.allocate(buffer_length);
            buffer.order(ByteOrder.LITTLE_ENDIAN); // Ubuntu byte order

            buffer.put("BeginOfADataFrame".getBytes());
            buffer.putInt(message_length);
            buffer.put(Timestamp.getBytes());
            buffer.put("_".getBytes());
            buffer.put(isDancing.getBytes());
            String Null = "\0";
            buffer.put(Null.getBytes());
            buffer.put(JPEG_length.getBytes());
            buffer.put(Null.getBytes());
            buffer.put(array_JPEG);
            buffer.put("EndOfADataFrame".getBytes());

            socketManager.sendImage(buffer);

        //2025/8/19 It does not work. I don't know why.
/*            RobotCommandOuterClass.RobotToServerMessage message =
                RobotCommandOuterClass.RobotToServerMessage.newBuilder()
                        .setDescription("ImageFrame")           //I cannot set the string. Why?
                        .setEventTime(time)
                        .setJpegdata(ByteString.copyFrom(array_JPEG))
                        .setJpegdatalength(array_JPEG.length)
                        .build();
            socketManager.sendImage(message);
 */
    }
}
