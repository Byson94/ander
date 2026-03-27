package android.view;

public class Window {

    private final View decorView = new View();

    public View getDecorView() {
        return decorView;
    }

    public void setFlags(int flags, int mask) {}
    public void addFlags(int flags) {}
    public void clearFlags(int flags) {}
}