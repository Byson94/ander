import com.badlogic.gdx.backends.lwjgl3.Lwjgl3Application;
import com.badlogic.gdx.backends.lwjgl3.Lwjgl3ApplicationConfiguration;
import com.badlogic.gdx.ApplicationListener;
import java.lang.reflect.Constructor;

public class AnderLauncherLwjgl3 {
    public static void main(String[] args) throws Exception {
        if (args.length > 0 && args[0].equals("--dry-run")) {
            Class.forName("com.badlogic.gdx.backends.lwjgl3.Lwjgl3Application");
            System.exit(0);
        }

        String mainClass = args[0];
        int width = Integer.parseInt(args[1]);
        int height = Integer.parseInt(args[2]);
        String title = args[3];

        Lwjgl3ApplicationConfiguration config = new Lwjgl3ApplicationConfiguration();
        config.setTitle(title);
        config.setWindowedMode(width, height);

        ApplicationListener instance = instantiate(mainClass);
        new Lwjgl3Application(instance, config);
    }

    private static ApplicationListener instantiate(String mainClass) throws Exception {
        Class<?> clazz = Class.forName(mainClass);

        // Strategy 1: no-arg constructor
        try {
            return (ApplicationListener) clazz.getDeclaredConstructor().newInstance();
        } catch (NoSuchMethodException ignored) {}

        // Strategy 2: single boolean arg (some games take a "debug" flag)
        try {
            Constructor<?> c = clazz.getDeclaredConstructor(boolean.class);
            c.setAccessible(true);
            return (ApplicationListener) c.newInstance(false);
        } catch (NoSuchMethodException ignored) {}

        // Strategy 3: pick the constructor with the fewest parameters and pass nulls/defaults
        Constructor<?> best = null;
        for (Constructor<?> c : clazz.getDeclaredConstructors()) {
            if (best == null || c.getParameterCount() < best.getParameterCount()) {
                best = c;
            }
        }

        if (best != null) {
            best.setAccessible(true);
            Object[] params = defaultParams(best.getParameterTypes());
            return (ApplicationListener) best.newInstance(params);
        }

        throw new RuntimeException("Could not instantiate " + mainClass + " — no usable constructor found");
    }

    private static Object[] defaultParams(Class<?>[] types) {
        Object[] params = new Object[types.length];
        for (int i = 0; i < types.length; i++) {
            Class<?> t = types[i];
            if      (t == boolean.class) params[i] = false;
            else if (t == int.class)     params[i] = 0;
            else if (t == long.class)    params[i] = 0L;
            else if (t == float.class)   params[i] = 0.0f;
            else if (t == double.class)  params[i] = 0.0;
            else                         params[i] = null;
        }
        return params;
    }
}