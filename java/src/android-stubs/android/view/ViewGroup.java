package android.view;

public class ViewGroup extends View {
    public ViewGroup() {}

    public static class LayoutParams {
        public int width;
        public int height;

        public LayoutParams(int width, int height) {
            this.width = width;
            this.height = height;
        }
    }
}