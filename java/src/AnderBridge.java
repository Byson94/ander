import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

public class AnderBridge {
    static {
        System.load(System.getProperty("ander.bridge.path"));
    }

    // Called from SharedLibraryLoader
    public static void registerNativesForClass(String className) throws Exception {
        Class<?> cls;
        try {
            cls = Class.forName(className);
        } catch (Throwable t) {
            System.err.println("[AnderBridge] skipping unloadable class: " + className + " — " + t.getMessage());
            return;
        }
        for (Method m : cls.getDeclaredMethods()) {
            if (!Modifier.isNative(m.getModifiers())) continue;
            String sig = buildSignature(m);
            System.err.println("[AnderBridge] registering: "
                + className + "." + m.getName() + " " + sig);
            registerNativeMethod(className, m.getName(), sig);
        }
    }

    private static String buildSignature(Method m) {
        StringBuilder sb = new StringBuilder("(");
        for (Class<?> p : m.getParameterTypes())
            sb.append(typeToSig(p));
        sb.append(")");
        sb.append(typeToSig(m.getReturnType()));
        return sb.toString();
    }

    private static String typeToSig(Class<?> t) {
        if (t == void.class)    return "V";
        if (t == boolean.class) return "Z";
        if (t == byte.class)    return "B";
        if (t == char.class)    return "C";
        if (t == short.class)   return "S";
        if (t == int.class)     return "I";
        if (t == long.class)    return "J";
        if (t == float.class)   return "F";
        if (t == double.class)  return "D";
        if (t.isArray())        return "[" + typeToSig(t.getComponentType());
        return "L" + t.getName().replace('.', '/') + ";";
    }

    // Implemented in AnderBridge.c
    public static native void connect();
    public static native void registerNativeMethod(String className,
                                                    String methodName,
                                                    String signature);
    public static native String[] listNativeSymbols();
}