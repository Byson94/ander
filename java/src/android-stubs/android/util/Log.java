package android.util;

/**
 * Desktop stub for android.util.Log which is redircted to desktop stdout/stderr.
 * LOG_LEVEL can be set to suppress lower-priority messages.
 */
public final class Log {

    public static final int VERBOSE = 2;
    public static final int DEBUG   = 3;
    public static final int INFO    = 4;
    public static final int WARN    = 5;
    public static final int ERROR   = 6;
    public static final int ASSERT  = 7;

    /** Minimum level to print. Change to INFO or WARN to reduce noise. */
    public static int LOG_LEVEL = VERBOSE;

    private Log() {}

    public static int v(String tag, String msg)                      { return print(VERBOSE, "V", tag, msg, null); }
    public static int v(String tag, String msg, Throwable tr)        { return print(VERBOSE, "V", tag, msg, tr);   }
    public static int d(String tag, String msg)                      { return print(DEBUG,   "D", tag, msg, null); }
    public static int d(String tag, String msg, Throwable tr)        { return print(DEBUG,   "D", tag, msg, tr);   }
    public static int i(String tag, String msg)                      { return print(INFO,    "I", tag, msg, null); }
    public static int i(String tag, String msg, Throwable tr)        { return print(INFO,    "I", tag, msg, tr);   }
    public static int w(String tag, String msg)                      { return print(WARN,    "W", tag, msg, null); }
    public static int w(String tag, String msg, Throwable tr)        { return print(WARN,    "W", tag, msg, tr);   }
    public static int w(String tag, Throwable tr)                    { return print(WARN,    "W", tag, "", tr);     }
    public static int e(String tag, String msg)                      { return print(ERROR,   "E", tag, msg, null); }
    public static int e(String tag, String msg, Throwable tr)        { return print(ERROR,   "E", tag, msg, tr);   }
    public static int wtf(String tag, String msg)                    { return print(ASSERT,  "A", tag, msg, null); }
    public static int wtf(String tag, String msg, Throwable tr)      { return print(ASSERT,  "A", tag, msg, tr);   }
    public static int wtf(String tag, Throwable tr)                  { return print(ASSERT,  "A", tag, "", tr);     }

    public static String getStackTraceString(Throwable tr) {
        if (tr == null) return "";
        java.io.StringWriter sw = new java.io.StringWriter();
        tr.printStackTrace(new java.io.PrintWriter(sw));
        return sw.toString();
    }

    public static boolean isLoggable(String tag, int level) { return level >= LOG_LEVEL; }

    private static int print(int level, String prefix, String tag, String msg, Throwable tr) {
        if (level < LOG_LEVEL) return 0;
        java.io.PrintStream out = (level >= WARN) ? System.err : System.out;
        out.printf("[%s/%s] %s%n", prefix, tag, msg);
        if (tr != null) tr.printStackTrace(out);
        return 0;
    }
}