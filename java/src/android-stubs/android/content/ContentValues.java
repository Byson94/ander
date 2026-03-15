package android.content;

import java.util.*;

/**
 * Desktop stub for android.content.ContentValues.
 * Thin wrapper around a LinkedHashMap (preserves insertion order for SQL).
 */
public class ContentValues {

    private final LinkedHashMap<String, Object> map;

    public ContentValues()                { map = new LinkedHashMap<>(); }
    public ContentValues(int capacity)    { map = new LinkedHashMap<>(capacity); }
    public ContentValues(ContentValues o) { map = new LinkedHashMap<>(o.map); }

    public void put(String key, String value)  { map.put(key, value); }
    public void put(String key, Integer value) { map.put(key, value); }
    public void put(String key, Long value)    { map.put(key, value); }
    public void put(String key, Float value)   { map.put(key, value); }
    public void put(String key, Double value)  { map.put(key, value); }
    public void put(String key, Boolean value) { map.put(key, value); }
    public void put(String key, Short value)   { map.put(key, value); }
    public void put(String key, Byte value)    { map.put(key, value); }
    public void put(String key, byte[] value)  { map.put(key, value); }
    public void putNull(String key)            { map.put(key, null); }

    public Object  get(String key)         { return map.get(key); }
    public String  getAsString(String key) { Object v = map.get(key); return v != null ? v.toString() : null; }

    public Integer getAsInteger(String key) {
        Object v = map.get(key);
        if (v instanceof Integer) return (Integer) v;
        try { return v != null ? Integer.parseInt(v.toString()) : null; }
        catch (NumberFormatException e) { return null; }
    }

    public Long getAsLong(String key) {
        Object v = map.get(key);
        if (v instanceof Long) return (Long) v;
        try { return v != null ? Long.parseLong(v.toString()) : null; }
        catch (NumberFormatException e) { return null; }
    }

    public Double getAsDouble(String key) {
        Object v = map.get(key);
        if (v instanceof Double) return (Double) v;
        try { return v != null ? Double.parseDouble(v.toString()) : null; }
        catch (NumberFormatException e) { return null; }
    }

    public Float getAsFloat(String key) {
        Object v = map.get(key);
        if (v instanceof Float) return (Float) v;
        try { return v != null ? Float.parseFloat(v.toString()) : null; }
        catch (NumberFormatException e) { return null; }
    }

    public Boolean getAsBoolean(String key) {
        Object v = map.get(key);
        if (v instanceof Boolean) return (Boolean) v;
        return v != null ? Boolean.parseBoolean(v.toString()) : null;
    }

    public byte[] getAsByteArray(String key) {
        Object v = map.get(key);
        return v instanceof byte[] ? (byte[]) v : null;
    }

    public boolean containsKey(String key)              { return map.containsKey(key); }
    public void    remove(String key)                   { map.remove(key); }
    public void    clear()                              { map.clear(); }
    public int     size()                               { return map.size(); }
    public boolean isEmpty()                            { return map.isEmpty(); }
    public Set<String> keySet()                         { return map.keySet(); }
    public Set<Map.Entry<String, Object>> valueSet()    { return map.entrySet(); }

    @Override public String toString() { return "ContentValues" + map; }
}