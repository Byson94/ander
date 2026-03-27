package android.os;

import java.util.concurrent.*;

public class Handler {

    private final ScheduledExecutorService executor;

    public Handler() {
        this.executor = Executors.newSingleThreadScheduledExecutor();
    }

    public boolean post(Runnable r) {
        executor.execute(r);
        return true;
    }

    public boolean postDelayed(Runnable r, long delayMillis) {
        executor.schedule(r, delayMillis, TimeUnit.MILLISECONDS);
        return true;
    }

    public void removeCallbacks(Runnable r) {
    }
}