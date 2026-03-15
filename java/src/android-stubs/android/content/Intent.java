package android.content;

import android.os.Bundle;
import java.util.HashMap;

/**
 * Desktop stub for android.content.Intent.
 * Stores action, extras, and flags; navigation calls are no-ops.
 */
public class Intent {

    // Common action constants
    public static final String ACTION_MAIN            = "android.intent.action.MAIN";
    public static final String ACTION_VIEW            = "android.intent.action.VIEW";
    public static final String ACTION_SEND            = "android.intent.action.SEND";
    public static final String ACTION_PICK            = "android.intent.action.PICK";
    public static final String ACTION_GET_CONTENT     = "android.intent.action.GET_CONTENT";
    public static final String EXTRA_TEXT             = "android.intent.extra.TEXT";
    public static final String EXTRA_SUBJECT          = "android.intent.extra.SUBJECT";
    public static final String EXTRA_EMAIL            = "android.intent.extra.EMAIL";

    public static final int FLAG_ACTIVITY_NEW_TASK          = 0x10000000;
    public static final int FLAG_ACTIVITY_CLEAR_TOP         = 0x04000000;
    public static final int FLAG_ACTIVITY_SINGLE_TOP        = 0x20000000;
    public static final int FLAG_ACTIVITY_NO_HISTORY        = 0x40000000;

    private String action;
    private String type;
    private String packageName;
    private Bundle extras = new Bundle();
    private int flags;

    public Intent() {}
    public Intent(String action) { this.action = action; }
    public Intent(String action, android.net.Uri uri) { this.action = action; }
    public Intent(Context ctx, Class<?> cls) {}

    // == Setters ==
    public Intent setAction(String action)   { this.action = action; return this; }
    public Intent setType(String type)       { this.type   = type;   return this; }
    public Intent setFlags(int flags)        { this.flags  = flags;  return this; }
    public Intent addFlags(int flags)        { this.flags |= flags;  return this; }
    public Intent setPackage(String pkg)     { this.packageName = pkg; return this; }

    public Intent putExtra(String name, String value)    { extras.putString(name, value);  return this; }
    public Intent putExtra(String name, int value)       { extras.putInt(name, value);     return this; }
    public Intent putExtra(String name, long value)      { extras.putLong(name, value);    return this; }
    public Intent putExtra(String name, float value)     { extras.putFloat(name, value);   return this; }
    public Intent putExtra(String name, double value)    { extras.putDouble(name, value);  return this; }
    public Intent putExtra(String name, boolean value)   { extras.putBoolean(name, value); return this; }
    public Intent putExtra(String name, String[] value)  { extras.putStringArray(name, value); return this; }
    public Intent putExtras(Bundle bundle)               { extras.putAll(bundle); return this; }

    // == Getters  =====
    public String  getAction()                               { return action; }
    public int     getFlags()                                { return flags; }
    public Bundle  getExtras()                               { return extras; }
    public boolean hasExtra(String name)                     { return extras.containsKey(name); }
    public String  getStringExtra(String name)               { return extras.getString(name); }
    public int     getIntExtra(String name, int def)         { return extras.getInt(name, def); }
    public long    getLongExtra(String name, long def)       { return extras.getLong(name, def); }
    public float   getFloatExtra(String name, float def)     { return extras.getFloat(name, def); }
    public double  getDoubleExtra(String name, double def)   { return extras.getDouble(name, def); }
    public boolean getBooleanExtra(String name, boolean def) { return extras.getBoolean(name, def); }
    public String[] getStringArrayExtra(String name)         { return extras.getStringArray(name); }

    @Override
    public String toString() {
        return "Intent{action=" + action + ", flags=" + flags + ", extras=" + extras + "}";
    }
}