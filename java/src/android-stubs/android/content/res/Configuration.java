package android.content.res;

import java.util.Locale;

public class Configuration {
    public float    fontScale         = 1.0f;
    public int      keyboard          = 0;
    public int      keyboardHidden    = 0;
    public Locale   locale            = Locale.getDefault();
    public int      mcc               = 0;
    public int      mnc               = 0;
    public int      navigation        = 0;
    public int      orientation       = 1; // ORIENTATION_PORTRAIT
    public int      screenLayout      = 0;
    public int      touchscreen       = 0;
    public int      uiMode            = 0;

    public static final int ORIENTATION_PORTRAIT  = 1;
    public static final int ORIENTATION_LANDSCAPE = 2;
}