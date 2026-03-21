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
            java.lang.reflect.Method reg = bridge.getMethod(
                "registerNativesForClass", String.class);

            String appJar = System.getProperty("ander.app.jar", "");
            if (appJar.isEmpty()) {
                System.err.println("[ander] ander.app.jar not set!");
                return;
            }

            System.out.println("[ander] scanning app jar: " + appJar);
            try (JarFile jar = new JarFile(appJar)) {
                Enumeration<JarEntry> entries = jar.entries();
                while (entries.hasMoreElements()) {
                    JarEntry je = entries.nextElement();
                    String name = je.getName();
                    if (!name.endsWith(".class")) continue;
                    String className = name.replace('/', '.').replace(".class", "");
                    try {
                        Class<?> cls = Class.forName(className);
                        boolean hasNative = false;
                        for (java.lang.reflect.Method m : cls.getDeclaredMethods()) {
                            if (Modifier.isNative(m.getModifiers())) {
                                hasNative = true;
                                break;
                            }
                        }
                        if (hasNative) {
                            System.out.println("[ander] registering native class: " + className);
                            reg.invoke(null, className);
                        }
                    } catch (Throwable t) {
                        // Skip unloadable classes silently
                    }
                }
            }
        } catch (Exception e) {
            throw new GdxRuntimeException("Failed to register natives for: " + libraryName, e);
        }
    }
}