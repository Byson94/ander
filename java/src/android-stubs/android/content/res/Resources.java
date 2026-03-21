package android.content.res;

public class Resources {

    public String getString(int resId)                    { return ""; }
    public String getString(int resId, Object... args)    { return ""; }
    public int    getInteger(int resId)                   { return 0;  }
    public float  getDimension(int resId)                 { return 0f; }
    public int    getColor(int resId)                     { return 0;  }
    public boolean getBoolean(int resId)                  { return false; }
    public String[] getStringArray(int resId)             { return new String[0]; }
    public int[]    getIntArray(int resId)                { return new int[0]; }
    public float  getDisplayMetrics() { return 1.0f; }

    public DisplayMetrics displayMetrics = new DisplayMetrics();

    public static class DisplayMetrics {
        public float density        = 1.0f;
        public float scaledDensity  = 1.0f;
        public float xdpi           = 96.0f;
        public float ydpi           = 96.0f;
        public int   widthPixels    = 1920;
        public int   heightPixels   = 1080;
        public int   densityDpi     = 96;
    }

    public Configuration configuration = new Configuration();

    public Configuration getConfiguration() {
        return configuration;
    }
}