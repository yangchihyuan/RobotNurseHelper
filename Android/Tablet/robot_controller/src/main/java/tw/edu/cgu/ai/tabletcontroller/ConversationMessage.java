package tw.edu.cgu.ai.tabletcontroller;

public class ConversationMessage {

    private String message;
    private boolean isRobot;

    public ConversationMessage(String message, boolean isRobot) {
        this.message = message;
        this.isRobot = isRobot;
    }

    public String getMessage() {
        return message;
    }

    public boolean isRobot() {
        return isRobot;
    }

    public void setMessage(String message) {
        this.message = message;
    }

    public void setRobot(boolean robot) {
        isRobot = robot;
    }
}