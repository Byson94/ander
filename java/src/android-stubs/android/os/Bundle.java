package android.os;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * Desktop stub for android.os.Bundle with a plain HashMap.
 */
public class Bundle {

    private final HashMap<String, Object> map = new HashMap<>();

    public Bundle() {}
    public Bundle(Bundle source) {
        if (source != null) map.putAll(source.map);
    }

    // == Putters  ==
    public void putString(String key, String value)   { map.put(key, value); }
    public void putInt(String key, int value)         { map.put(key, value); }
    public void putLong(String key, long value)       { map.put(key, value); }
    public void putFloat(String key, float value)     { map.put(key, value); }
    public void putDouble(String key, double value)   { map.put(key, value); }
    public void putBoolean(String key, boolean value) { map.put(key, value); }
    public void putByte(String key, byte value)       { map.put(key, value); }
    public void putChar(String key, char value)       { map.put(key, value); }
    public void putShort(String key, short value)     { map.put(key, value); }
    public void putStringArray(String key, String[] value) { map.put(key, value); }
    public void putIntArray(String key, int[] value)       { map.put(key, value); }
    public void putSerializable(String key, java.io.Serializable value) { map.put(key, value); }
    public void putBundle(String key, Bundle value)   { map.put(key, value); }
    public void putAll(Bundle bundle)                 { map.putAll(bundle.map); }

    // == Getters  ==
    public String  getString(String key)                       { return getString(key, null); }
    public String  getString(String key, String def)           { Object v = map.get(key); return v instanceof String  ? (String)  v : def; }
    public int     getInt(String key)                          { return getInt(key, 0); }
    public int     getInt(String key, int def)                 { Object v = map.get(key); return v instanceof Integer ? (Integer) v : def; }
    public long    getLong(String key)                         { return getLong(key, 0L); }
    public long    getLong(String key, long def)               { Object v = map.get(key); return v instanceof Long    ? (Long)    v : def; }
    public float   getFloat(String key)                        { return getFloat(key, 0f); }
    public float   getFloat(String key, float def)             { Object v = map.get(key); return v instanceof Float   ? (Float)   v : def; }
    public double  getDouble(String key)                       { return getDouble(key, 0.0); }
    public double  getDouble(String key, double def)           { Object v = map.get(key); return v instanceof Double  ? (Double)  v : def; }
    public boolean getBoolean(String key)                      { return getBoolean(key, false); }
    public boolean getBoolean(String key, boolean def)         { Object v = map.get(key); return v instanceof Boolean ? (Boolean) v : def; }
    public Bundle  getBundle(String key)                       { Object v = map.get(key); return v instanceof Bundle  ? (Bundle)  v : null; }
    public String[] getStringArray(String key)                 { Object v = map.get(key); return v instanceof String[] ? (String[]) v : null; }
    public int[]    getIntArray(String key)                    { Object v = map.get(key); return v instanceof int[]    ? (int[])    v : null; }

    @SuppressWarnings("unchecked")
    public <T extends java.io.Serializable> T getSerializable(String key) {
        Object v = map.get(key);
        try { return (T) v; } catch (ClassCastException e) { return null; }
    }

    // == Utility  ==
    public boolean containsKey(String key) { return map.containsKey(key); }
    public void    remove(String key)      { map.remove(key); }
    public void    clear()                 { map.clear(); }
    public boolean isEmpty()               { return map.isEmpty(); }
    public int     size()                  { return map.size(); }
    public Set<String> keySet()            { return map.keySet(); }

    @Override
    public String toString() { return "Bundle" + map.toString(); }
}