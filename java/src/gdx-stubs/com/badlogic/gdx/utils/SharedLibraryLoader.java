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
        if (sym == null || !sym.startsWith("Java_")) return null;

        String body = sym.substring(5);

        int overloadSep = body.indexOf("__");
        if (overloadSep >= 0) {
            body = body.substring(0, overloadSep);
        }

        java.util.List<String> parts = splitMangledParts(body);
        if (parts.size() < 2) return null;

        parts.remove(parts.size() - 1);
        return String.join(".", parts);
    }

    private java.util.List<String> splitMangledParts(String mangled) {
        java.util.List<String> parts = new java.util.ArrayList<>();
        StringBuilder current = new StringBuilder();
        int i = 0;
        while (i < mangled.length()) {
            char c = mangled.charAt(i);
            if (c == '_') {
                char next = i + 1 < mangled.length() ? mangled.charAt(i + 1) : 0;
                if (next == '1') { current.append('_'); i += 2; continue; }
                if (next == '2') { current.append(';'); i += 2; continue; }
                if (next == '3') { current.append('['); i += 2; continue; }
                if (next == '0' && i + 5 < mangled.length()) {
                    String hex = mangled.substring(i + 2, i + 6);
                    if (hex.matches("[0-9a-fA-F]{4}")) {
                        current.append((char) Integer.parseInt(hex, 16));
                        i += 6; continue;
                    }
                }
                parts.add(current.toString());
                current.setLength(0);
                i++;
            } else {
                current.append(c);
                i++;
            }
        }
        if (current.length() > 0) parts.add(current.toString());
        return parts;
    }
}