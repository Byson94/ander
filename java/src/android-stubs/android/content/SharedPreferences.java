package android.content;

import java.io.*;
import java.util.*;

/**
 * Desktop stub for android.content.SharedPreferences.
 * Backed by a .properties file on disk; changes are committed synchronously.
 */
public class SharedPreferences {

    private final File file;
    private final Properties props = new Properties();

    public SharedPreferences(File file) {
        this.file = file;
        load();
    }

    // == Read  ==
    public String  getString(String key, String defValue)   { return props.getProperty(key, defValue != null ? defValue : ""); }
    public int     getInt(String key, int defValue)         { try { return Integer.parseInt(props.getProperty(key, String.valueOf(defValue))); } catch (NumberFormatException e) { return defValue; } }
    public long    getLong(String key, long defValue)       { try { return Long.parseLong(props.getProperty(key, String.valueOf(defValue))); }    catch (NumberFormatException e) { return defValue; } }
    public float   getFloat(String key, float defValue)     { try { return Float.parseFloat(props.getProperty(key, String.valueOf(defValue))); }  catch (NumberFormatException e) { return defValue; } }
    public boolean getBoolean(String key, boolean defValue) { String v = props.getProperty(key); return v != null ? Boolean.parseBoolean(v) : defValue; }

    public boolean contains(String key) { return props.containsKey(key); }

    public Map<String, ?> getAll() {
        Map<String, String> all = new HashMap<>();
        for (String k : props.stringPropertyNames()) all.put(k, props.getProperty(k));
        return Collections.unmodifiableMap(all);
    }

    // == Editor  ==
    public Editor edit() { return new Editor(); }

    public class Editor {

        public Editor putString(String key, String value)   { props.setProperty(key, value != null ? value : ""); return this; }
        public Editor putInt(String key, int value)         { props.setProperty(key, String.valueOf(value));  return this; }
        public Editor putLong(String key, long value)       { props.setProperty(key, String.valueOf(value));  return this; }
        public Editor putFloat(String key, float value)     { props.setProperty(key, String.valueOf(value));  return this; }
        public Editor putBoolean(String key, boolean value) { props.setProperty(key, String.valueOf(value));  return this; }
        public Editor remove(String key)                    { props.remove(key); return this; }
        public Editor clear()                               { props.clear();     return this; }

        /** Writes to disk synchronously (same as commit on desktop). */
        public boolean commit() {
            save();
            return true;
        }

        /** Identical to commit() on desktop (no async thread needed). */
        public void apply() { save(); }
    }

    // == Listeners Stub ==
    public interface OnSharedPreferenceChangeListener {
        void onSharedPreferenceChanged(SharedPreferences prefs, String key);
    }
    public void registerOnSharedPreferenceChangeListener(OnSharedPreferenceChangeListener l)   {}
    public void unregisterOnSharedPreferenceChangeListener(OnSharedPreferenceChangeListener l) {}

    // == I/O helpers ==
    private void load() {
        if (!file.exists()) return;
        try (InputStream in = new FileInputStream(file)) {
            props.load(in);
        } catch (IOException e) {
            System.err.println("[SharedPreferences] Failed to load " + file + ": " + e.getMessage());
        }
    }

    private void save() {
        try {
            file.getParentFile().mkdirs();
            try (OutputStream out = new FileOutputStream(file)) {
                props.store(out, "android.content.SharedPreferences desktop stub");
            }
        } catch (IOException e) {
            System.err.println("[SharedPreferences] Failed to save " + file + ": " + e.getMessage());
        }
    }
}