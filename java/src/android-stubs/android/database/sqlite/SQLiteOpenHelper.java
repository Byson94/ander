package android.database.sqlite;

import android.content.Context;

import java.io.File;

/**
 * Desktop stub for android.database.sqlite.SQLiteOpenHelper.
 * Handles creation and version-upgrade lifecycle just like on Android.
 */
public abstract class SQLiteOpenHelper {

    private final Context context;
    private final String  name;
    private final int     version;

    private SQLiteDatabase writeableDb;
    private SQLiteDatabase readableDb;

    public SQLiteOpenHelper(Context context, String name, Object factory, int version) {
        this.context = context;
        this.name    = name;
        this.version = version;
    }

    // == Subclass contract ==
    public abstract void onCreate(SQLiteDatabase db);
    public abstract void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion);
    public void onDowngrade(SQLiteDatabase db, int oldVersion, int newVersion) {}
    public void onOpen(SQLiteDatabase db) {}

    // == Accessors  ==
    public synchronized SQLiteDatabase getWritableDatabase() {
        if (writeableDb != null && writeableDb.isOpen()) return writeableDb;
        writeableDb = openAndMigrate();
        return writeableDb;
    }

    public synchronized SQLiteDatabase getReadableDatabase() {
        if (readableDb != null && readableDb.isOpen()) return readableDb;
        readableDb = openAndMigrate();
        return readableDb;
    }

    public synchronized void close() {
        if (writeableDb != null) { writeableDb.close(); writeableDb = null; }
        if (readableDb  != null) { readableDb.close();  readableDb  = null; }
    }

    public String getDatabaseName() { return name; }

    // == Internal  ==
    private SQLiteDatabase openAndMigrate() {
        File dbFile = context.getDatabasePath(name);
        boolean isNew = !dbFile.exists();

        SQLiteDatabase db = SQLiteDatabase.openOrCreateDatabase(dbFile, null);

        int storedVersion = 0;
        try (android.database.Cursor c = db.rawQuery("PRAGMA user_version", null)) {
            if (c.moveToNext()) storedVersion = c.getInt(0);
        }

        if (isNew || storedVersion == 0) {
            onCreate(db);
            db.execSQL("PRAGMA user_version = " + version);
        } else if (storedVersion < version) {
            onUpgrade(db, storedVersion, version);
            db.execSQL("PRAGMA user_version = " + version);
        } else if (storedVersion > version) {
            onDowngrade(db, storedVersion, version);
            db.execSQL("PRAGMA user_version = " + version);
        }

        onOpen(db);
        return db;
    }
}