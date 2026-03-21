package com.badlogic.gdx.utils;

import java.util.jar.JarFile;
import java.util.jar.JarEntry;
import java.util.Enumeration;
import java.lang.reflect.Modifier;

public class SharedLibraryLoader {
    public static boolean isWindows = false;
    public static boolean isLinux   = true;
    public static boolean isMac     = false;
    public static boolean isARM     = false;
    public static boolean is64Bit   = true;

    private static boolean registered = false;

    public SharedLibraryLoader() {}
    public SharedLibraryLoader(String nativesJar) {}

    public void load(String libraryName) {
        System.out.println("[ander] SharedLibraryLoader.load(): " + libraryName);
        if (registered) return;
        registered = true;

        try {
            Class<?> bridge = Class.forName("AnderBridge");
            java.lang.reflect.Method listSymbols = bridge.getMethod("listNativeSymbols");
            java.lang.reflect.Method regClass = bridge.getMethod("registerNativesForClass", String.class);

            String[] symbols = (String[]) listSymbols.invoke(null);
            if (symbols == null || symbols.length == 0) {
                System.err.println("[ander] no Java_* symbols from launcher");
                return;
            }

            java.util.LinkedHashSet<String> classes = new java.util.LinkedHashSet<>();
            for (String sym : symbols) {
                String cls = symbolToClassName(sym);
                if (cls != null) classes.add(cls);
            }

            for (String className : classes) {
                System.out.println("[ander] registering: " + className);
                regClass.invoke(null, className);
            }

        } catch (Exception e) {
            throw new GdxRuntimeException("Failed to register natives for: " + libraryName, e);
        }
    }

    private String symbolToClassName(String sym) {
        if (!sym.startsWith("Java_")) return null;
        String[] parts = sym.substring(5).split("_");
        StringBuilder cls = new StringBuilder();
        for (String part : parts) {
            if (part.isEmpty()) continue;
            if (Character.isUpperCase(part.charAt(0))) {
                if (cls.length() > 0) cls.append('.');
                cls.append(part);
                break;
            }
            if (cls.length() > 0) cls.append('.');
            cls.append(part);
        }
        return cls.length() > 0 ? cls.toString() : null;
    }
}