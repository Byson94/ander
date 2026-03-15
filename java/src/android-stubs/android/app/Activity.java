package android.app;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

/**
 * Desktop stub for android.app.Activity.
 * All lifecycle methods are no-ops. Extend this class just as you would on Android.
 */
public class Activity extends Context {

    // lifecycle of app
    protected void onCreate(Bundle savedInstanceState) {}
    protected void onStart()   {}
    protected void onResume()  {}
    protected void onPause()   {}
    protected void onStop()    {}
    protected void onDestroy() {}
    protected void onRestart() {}

    protected void onSaveInstanceState(Bundle outState)              {}
    protected void onRestoreInstanceState(Bundle savedInstanceState) {}

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {}

    // Window / UI stubs
    public void setContentView(int layoutResID) {}
    public void setContentView(Object view)     {}
    public void setTitle(CharSequence title)    {}
    public void setTitle(int titleId)           {}

    @SuppressWarnings("unchecked")
    public <T> T findViewById(int id) { return null; }

    public void runOnUiThread(Runnable action) {
        // On desktop just run synchronously
        action.run();
    }

    // Navigation Stubs
    public void startActivity(Intent intent) {}
    public void startActivityForResult(Intent intent, int requestCode) {}
    public void finish() {}

    public static final int RESULT_OK       = -1;
    public static final int RESULT_CANCELED =  0;
    public void setResult(int resultCode) {}
    public void setResult(int resultCode, Intent data) {}
}