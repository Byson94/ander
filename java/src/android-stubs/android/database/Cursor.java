package android.database;

import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.PreparedStatement;

/**
 * Desktop stub for android.database.Cursor which wraps a JDBC ResultSet.
 * Call close() when done to release the underlying statement.
 */
public class Cursor implements AutoCloseable {

    private final ResultSet rs;
    private final PreparedStatement ps;
    private final ResultSetMetaData meta;
    private boolean beforeFirst = true;

    public Cursor(ResultSet rs, PreparedStatement ps) {
        this.rs = rs;
        this.ps = ps;
        try { this.meta = rs.getMetaData(); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    // == Navigation  ==
    public boolean moveToNext() {
        try { beforeFirst = false; return rs.next(); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public boolean moveToFirst() {
        if (!beforeFirst) throw new UnsupportedOperationException(
            "moveToFirst after iteration not supported with forward-only ResultSet");
        return moveToNext();
    }

    public boolean isAfterLast() {
        try { return rs.isAfterLast(); } catch (SQLException e) { return true; }
    }

    public boolean isClosed() {
        try { return rs.isClosed(); } catch (SQLException e) { return true; }
    }

    // == Column info ==
    public int getColumnCount() {
        try { return meta.getColumnCount(); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public String getColumnName(int columnIndex) {
        try { return meta.getColumnLabel(columnIndex + 1); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public int getColumnIndex(String columnName) {
        try {
            for (int i = 1; i <= meta.getColumnCount(); i++)
                if (meta.getColumnLabel(i).equalsIgnoreCase(columnName)) return i - 1;
            return -1;
        } catch (SQLException e) { throw new RuntimeException(e); }
    }

    public int getColumnIndexOrThrow(String columnName) {
        int idx = getColumnIndex(columnName);
        if (idx == -1) throw new IllegalArgumentException("No column named: " + columnName);
        return idx;
    }

    // == Data access (columnIndex is 0-based, JDBC is 1-based) ==
    public String getString(int columnIndex) {
        try { return rs.getString(columnIndex + 1); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public int getInt(int columnIndex) {
        try { return rs.getInt(columnIndex + 1); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public long getLong(int columnIndex) {
        try { return rs.getLong(columnIndex + 1); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public float getFloat(int columnIndex) {
        try { return rs.getFloat(columnIndex + 1); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public double getDouble(int columnIndex) {
        try { return rs.getDouble(columnIndex + 1); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public byte[] getBlob(int columnIndex) {
        try { return rs.getBytes(columnIndex + 1); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public boolean isNull(int columnIndex) {
        try { rs.getObject(columnIndex + 1); return rs.wasNull(); }
        catch (SQLException e) { throw new RuntimeException(e); }
    }

    public static final int FIELD_TYPE_NULL    = 0;
    public static final int FIELD_TYPE_INTEGER = 1;
    public static final int FIELD_TYPE_FLOAT   = 2;
    public static final int FIELD_TYPE_STRING  = 3;
    public static final int FIELD_TYPE_BLOB    = 4;

    public int getType(int columnIndex) {
        try {
            String t = meta.getColumnTypeName(columnIndex + 1).toUpperCase();
            if (t.contains("INT"))                                      return FIELD_TYPE_INTEGER;
            if (t.contains("REAL") || t.contains("FLOAT")
                                   || t.contains("DOUBLE"))             return FIELD_TYPE_FLOAT;
            if (t.contains("BLOB"))                                     return FIELD_TYPE_BLOB;
            if (t.equals("NULL"))                                       return FIELD_TYPE_NULL;
            return FIELD_TYPE_STRING;
        } catch (SQLException e) { return FIELD_TYPE_STRING; }
    }

    // == Close  ==
    public void close() {
        try { rs.close(); } catch (SQLException e) { /* ignore */ }
        try { if (ps != null) ps.close(); } catch (SQLException e) { /* ignore */ }
    }
}