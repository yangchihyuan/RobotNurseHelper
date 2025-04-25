package tw.edu.cgu.ai.zenbo;
import android.os.Handler;
import android.os.Message;

public class ExecuteCommandCallback implements Handler.Callback {
    public boolean handleMessage(Message msg)
    {
        //How to let the handler wait until the callback function check Zenbo SDK's return?
        return true;
    }
}
