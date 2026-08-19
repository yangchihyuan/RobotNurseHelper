package tw.edu.cgu.ai.tabletcontroller;

public class CataractSession {

    private int currentStep = 0;

    private final String[] englishLessons = {

            "Welcome! Today we will learn about cataracts.",

            "A cataract is the clouding of the eye's natural lens.",

            "Common symptoms include blurred vision, glare, and difficulty seeing at night.",

            "Risk factors include aging, diabetes, smoking, and prolonged UV exposure.",

            "Doctors diagnose cataracts using a comprehensive eye examination.",

            "Treatment involves cataract surgery to replace the cloudy lens with an artificial lens.",

            "Protect your eyes by wearing sunglasses, eating healthy, and getting regular eye checkups.",

            "Thank you for completing today's cataract education session."

    };

    public String getCurrentLesson() {

        return englishLessons[currentStep];

    }

    public boolean nextLesson() {

        if(currentStep < englishLessons.length - 1){

            currentStep++;

            return true;

        }

        return false;

    }

    public boolean previousLesson() {

        if(currentStep > 0){

            currentStep--;

            return true;

        }

        return false;

    }

    public void restartSession() {

        currentStep = 0;

    }

    public int getCurrentStep() {

        return currentStep;

    }

    public int getTotalSteps() {

        return englishLessons.length;

    }

}