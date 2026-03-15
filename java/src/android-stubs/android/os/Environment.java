package android.os;

import java.io.File;

/**
 * Desktop stub for android.os.Environment.
 * Redirects storage paths to ~/.local/share/<appname> (XDG-compatible).
 * App name is read from the system property "ander.app.name".
 */
public final class Environment {

    public static final String DIRECTORY_MUSIC         = "Music";
    public static final String DIRECTORY_PODCASTS      = "Podcasts";
    public static final String DIRECTORY_RINGTONES     = "Ringtones";
    public static final String DIRECTORY_ALARMS        = "Alarms";
    public static final String DIRECTORY_NOTIFICATIONS = "Notifications";
    public static final String DIRECTORY_PICTURES      = "Pictures";
    public static final String DIRECTORY_MOVIES        = "Movies";
    public static final String DIRECTORY_DOWNLOADS     = "Downloads";
    public static final String DIRECTORY_DCIM          = "DCIM";
    public static final String DIRECTORY_DOCUMENTS     = "Documents";

    private Environment() {}

    public static String getAppName() {
        return System.getProperty("ander.app.name", "android-desktop-app");
    }

    /** ~/.local/share/ander/<appname> */
    public static File getDataDirectory() {
        File dir = new File(xdgDataHome(), "ander/" + getAppName());
        dir.mkdirs();
        return dir;
    }

    /** ~/.local/share/ander/<appname>/external */
    public static File getExternalStorageDirectory() {
        File dir = new File(xdgDataHome(), "ander/" + getAppName() + "/external");
        dir.mkdirs();
        return dir;
    }

    public static File getExternalStoragePublicDirectory(String type) {
        File dir = new File(getExternalStorageDirectory(), type != null ? type : "");
        dir.mkdirs();
        return dir;
    }

    public static boolean isExternalStorageEmulated()  { return true;  }
    public static boolean isExternalStorageRemovable() { return false; }
    public static String  getExternalStorageState()    { return "mounted"; }

    // == Helpers ==
    private static File xdgDataHome() {
        String xdg = System.getenv("XDG_DATA_HOME");
        if (xdg != null && !xdg.isEmpty()) return new File(xdg);
        return new File(System.getProperty("user.home"), ".local/share");
    }
}