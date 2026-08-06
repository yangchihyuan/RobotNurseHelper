package tw.edu.cgu.ai.kebbi

import android.Manifest
import android.content.pm.PackageManager
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log
import android.os.Handler
import androidx.annotation.RequiresPermission
import androidx.core.app.ActivityCompat

class AudioManager {
    //    private int audioSouce = MediaRecorder.AudioSource.MIC;
    private val audioSouce = MediaRecorder.AudioSource.VOICE_RECOGNITION

    //    private int sampleRate = 44100 ; // 44100 for music, however, for whisper, the sample rate is 16000
    private val sampleRate = 16000

    //    private int channelConfig = AudioFormat.CHANNEL_IN_STEREO;  //for whisper, the channel is mono
    private val channelConfig = AudioFormat.CHANNEL_IN_MONO //for whisper, the channel is mono
    private val audioFormat = AudioFormat.ENCODING_PCM_16BIT
    var minBufSize: Int = AudioRecord.getMinBufferSize(
        sampleRate,
        channelConfig,
        audioFormat
    ) //minBufSize = 5376, but larger is better.
    private val status = true

    var recorder: AudioRecord? = null

    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    fun startRecording()
    {
        recorder = AudioRecord(
            audioSouce,
            sampleRate,
            channelConfig,
            audioFormat,
            minBufSize * 10
        ) //5376* 10
        Log.d("VS", "Recorder initialized")

        recorder?.startRecording();    //The recorder means the audio recorder
    }

    fun stopRecording()
    {
        recorder?.stop()
    }

    fun startTransmitToServer(handlerSendAudio: Handler, socketManager: SocketManager) {
        //Start audio recorder
        handlerSendAudio.post(
            object : Runnable {
                override fun run() {
                    val buffer = ShortArray(minBufSize) //minBufSize = 5376
                    Log.d(
                        "VS",
                        "Buffer created of size " + minBufSize
                    ) // every 5 second, the log message occurs. It does not make sense.

                    var readSize: Int
                    while (status) {
                        //reading data from MIC into buffer
                        readSize = recorder!!.read(buffer, 0, buffer.size)
                        if (readSize >= 0) {
                            val byteBuffer = Converter.ShortToByte_Twiddle_Method(buffer)
                            socketManager.sendAudio(byteBuffer)
                        } else {
                            Log.e("recorder", "recorder.read() error")
                        }
                    }
                }
            }
        )
    }

}