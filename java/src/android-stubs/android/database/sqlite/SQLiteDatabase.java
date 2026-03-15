package android.database.sqlite;

import android.content.ContentValues;
import android.database.Cursor;

import java.sql.*;
import java.util.*;

/**
 * Desktop stub for android.database.sqlite.SQLiteDatabase.
 * Backed by xerial sqlite-jdbc (org.xerial:sqlite-jdbc).
 *
 * Add sqlite-jdbc to your classpath:
 *   Maven: org.xerial:sqlite-jdbc:3.45.x
 *   Arch:  pacman -S sqlite-jdbc  (or grab the jar from Maven Central)
 */
public class SQLiteDatabase {

    public static final int OPEN_READWRITE   = 0;
    public static final int OPEN_READONLY    = 1;
    public static final int CREATE_IF_NEEDED = 268435456;

    public static final int CONFLICT_NONE     = 0;
    public static final int CONFLICT_ROLLBACK = 1;
    public static final int CONFLICT_ABORT    = 2;
    public static final int CONFLICT_FAIL     = 3;
    public static final int CONFLICT_IGNORE   = 4;
    public static final int CONFLICT_REPLACE  = 5;

    final Connection conn;
    private final String path;

    private SQLiteDatabase(String path) throws SQLException {
        this.path = path;
        this.conn = DriverManager.getConnection("jdbc:sqlite:" + path);
        this.conn.setAutoCommit(true);
    }

    // == Open / close ==
    public static SQLiteDatabase openOrCreateDatabase(String path, Object factory) {
        return openOrCreateDatabase(new java.io.File(path), factory);
    }

    public static SQLiteDatabase openOrCreateDatabase(java.io.File file, Object factory) {
        try {
            Class.forName("org.sqlite.JDBC");
            file.getParentFile().mkdirs();
            return new SQLiteDatabase(file.getAbsolutePath());
        } catch (ClassNotFoundException e) {
            throw new RuntimeException("sqlite-jdbc not on classpath. Add org.xerial:sqlite-jdbc.", e);
        } catch (SQLException e) {
            throw new SQLiteException("Cannot open database: " + e.getMessage(), e);
        }
    }

    public boolean isOpen() {
        try { return conn != null && !conn.isClosed(); } catch (SQLException e) { return false; }
    }

    public void close() {
        try { if (isOpen()) conn.close(); } catch (SQLException e) { /* ignore */ }
    }

    public String getPath() { return path; }

    // == DDL  ==
    public void execSQL(String sql) {
        try (Statement st = conn.createStatement()) { st.execute(sql); }
        catch (SQLException e) { throw new SQLiteException(sql, e); }
    }

    public void execSQL(String sql, Object[] bindArgs) {
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            bind(ps, bindArgs);
            ps.execute();
        } catch (SQLException e) { throw new SQLiteException(sql, e); }
    }

    // == Query  ==
    public Cursor query(String table, String[] columns, String selection,
                        String[] selectionArgs, String groupBy,
                        String having, String orderBy) {
        return query(table, columns, selection, selectionArgs, groupBy, having, orderBy, null);
    }

    public Cursor query(String table, String[] columns, String selection,
                        String[] selectionArgs, String groupBy,
                        String having, String orderBy, String limit) {
        String cols = (columns == null || columns.length == 0) ? "*" : String.join(", ", columns);
        StringBuilder sb = new StringBuilder("SELECT ").append(cols).append(" FROM ").append(table);
        if (selection != null && !selection.isEmpty())  sb.append(" WHERE ").append(selection);
        if (groupBy   != null && !groupBy.isEmpty())    sb.append(" GROUP BY ").append(groupBy);
        if (having    != null && !having.isEmpty())     sb.append(" HAVING ").append(having);
        if (orderBy   != null && !orderBy.isEmpty())    sb.append(" ORDER BY ").append(orderBy);
        if (limit     != null && !limit.isEmpty())      sb.append(" LIMIT ").append(limit);
        return rawQuery(sb.toString(), selectionArgs);
    }

    public Cursor rawQuery(String sql, String[] selectionArgs) {
        try {
            PreparedStatement ps = conn.prepareStatement(sql);
            if (selectionArgs != null)
                for (int i = 0; i < selectionArgs.length; i++)
                    ps.setString(i + 1, selectionArgs[i]);
            return new Cursor(ps.executeQuery(), ps);
        } catch (SQLException e) { throw new SQLiteException(sql, e); }
    }

    // == DML  ==
    public long insert(String table, String nullColumnHack, ContentValues values) {
        return insertWithOnConflict(table, nullColumnHack, values, CONFLICT_NONE);
    }

    public long insertOrThrow(String table, String nullColumnHack, ContentValues values) {
        return insert(table, nullColumnHack, values);
    }

    public long insertWithOnConflict(String table, String nullColumnHack,
                                     ContentValues values, int conflictAlgorithm) {
        String conflict = conflictClause(conflictAlgorithm);
        Set<String> keys = values.keySet();
        String cols = String.join(", ", keys);
        String placeholders = String.join(", ", Collections.nCopies(keys.size(), "?"));
        String sql = "INSERT " + conflict + "INTO " + table
                   + " (" + cols + ") VALUES (" + placeholders + ")";
        try (PreparedStatement ps = conn.prepareStatement(sql, Statement.RETURN_GENERATED_KEYS)) {
            int i = 1;
            for (String k : keys) ps.setObject(i++, values.get(k));
            ps.executeUpdate();
            try (ResultSet rs = ps.getGeneratedKeys()) { return rs.next() ? rs.getLong(1) : -1; }
        } catch (SQLException e) { throw new SQLiteException(sql, e); }
    }

    public int update(String table, ContentValues values,
                      String whereClause, String[] whereArgs) {
        return updateWithOnConflict(table, values, whereClause, whereArgs, CONFLICT_NONE);
    }

    public int updateWithOnConflict(String table, ContentValues values,
                                    String whereClause, String[] whereArgs,
                                    int conflictAlgorithm) {
        String conflict = conflictClause(conflictAlgorithm);
        Set<String> keys = values.keySet();
        String sets = String.join(", ", keys.stream()
                          .map(k -> k + " = ?").toArray(String[]::new));
        StringBuilder sb = new StringBuilder("UPDATE ").append(conflict)
                .append(table).append(" SET ").append(sets);
        if (whereClause != null && !whereClause.isEmpty())
            sb.append(" WHERE ").append(whereClause);
        String sql = sb.toString();
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            int i = 1;
            for (String k : keys)  ps.setObject(i++, values.get(k));
            if (whereArgs != null) for (String a : whereArgs) ps.setString(i++, a);
            return ps.executeUpdate();
        } catch (SQLException e) { throw new SQLiteException(sql, e); }
    }

    public int delete(String table, String whereClause, String[] whereArgs) {
        StringBuilder sb = new StringBuilder("DELETE FROM ").append(table);
        if (whereClause != null && !whereClause.isEmpty())
            sb.append(" WHERE ").append(whereClause);
        String sql = sb.toString();
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            if (whereArgs != null)
                for (int i = 0; i < whereArgs.length; i++) ps.setString(i + 1, whereArgs[i]);
            return ps.executeUpdate();
        } catch (SQLException e) { throw new SQLiteException(sql, e); }
    }

    // == Transactions ==
    public void beginTransaction() {
        try { conn.setAutoCommit(false); }
        catch (SQLException e) { throw new SQLiteException("beginTransaction", e); }
    }

    public void setTransactionSuccessful() { /* commit deferred to endTransaction */ }

    public void endTransaction() {
        try { conn.commit(); conn.setAutoCommit(true); }
        catch (SQLException e) {
            try { conn.rollback(); } catch (SQLException ex) { /* ignore */ }
            throw new SQLiteException("endTransaction", e);
        }
    }

    public boolean inTransaction() {
        try { return !conn.getAutoCommit(); } catch (SQLException e) { return false; }
    }

    // == Helpers  ==
    private static String conflictClause(int algorithm) {
        switch (algorithm) {
            case CONFLICT_REPLACE:  return "OR REPLACE ";
            case CONFLICT_IGNORE:   return "OR IGNORE ";
            case CONFLICT_ROLLBACK: return "OR ROLLBACK ";
            case CONFLICT_ABORT:    return "OR ABORT ";
            case CONFLICT_FAIL:     return "OR FAIL ";
            default:                return "";
        }
    }

    private static void bind(PreparedStatement ps, Object[] args) throws SQLException {
        if (args == null) return;
        for (int i = 0; i < args.length; i++) ps.setObject(i + 1, args[i]);
    }
}