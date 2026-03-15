package android.content;

import android.content.SharedPreferences;
import android.os.Environment;

import java.io.File;

/**
 * Desktop stub for android.content.Context.
 * File-path methods redirect to ~/.local/share/<appname> (via Environment).
 */
public class Context {

    public static final int MODE_PRIVATE       = 0;
    public static final int MODE_APPEND        = 32768;
    public static final int MODE_WORLD_READABLE  = 1;
    public static final int MODE_WORLD_WRITEABLE = 2;

    // == Files  ==
    public File getFilesDir() {
        File dir = new File(Environment.getDataDirectory(), "files");
        dir.mkdirs();
        return dir;
    }

    public File getCacheDir() {
        File dir = new File(Environment.getDataDirectory(), "cache");
        dir.mkdirs();
        return dir;
    }

    public File getExternalFilesDir(String type) {
        File base = new File(Environment.getExternalStorageDirectory(),
                             type != null ? type : "");
        base.mkdirs();
        return base;
    }

    public File getDatabasePath(String name) {
        File dir = new File(Environment.getDataDirectory(), "databases");
        dir.mkdirs();
        return new File(dir, name);
    }

    public File getFileStreamPath(String name) {
        return new File(getFilesDir(), name);
    }

    // == SharedPreferences ==
    public SharedPreferences getSharedPreferences(String name, int mode) {
        File prefsDir = new File(Environment.getDataDirectory(), "shared_prefs");
        prefsDir.mkdirs();
        return new SharedPreferences(new File(prefsDir, name + ".properties"));
    }

    // == Resources / package (stubs) ==
    public String getPackageName() { return "com.desktop.stub"; }
    public String getString(int resId) { return ""; }
    public String getString(int resId, Object... formatArgs) { return ""; }
    public int    getColor(int resId)  { return 0; }

    // System services (stub) 
    public Object getSystemService(String name) { return null; }
}