package android.widget;

import android.view.ViewGroup;

public class FrameLayout extends ViewGroup {

    public FrameLayout() {}

    public static class LayoutParams extends ViewGroup.LayoutParams {
        public int gravity;

        public LayoutParams(int width, int height) {
            super(width, height);
        }
    }
}