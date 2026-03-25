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

        // Decode JNI mangling: _00024 -> $, _1 -> _, then split on _ for dots
        String decoded = decodeJniName(body);

        int lastDot = decoded.lastIndexOf('_');
        int lastSep = decoded.lastIndexOf('.');
        if (lastSep < 0) return null;

        return decoded.substring(0, lastSep);
    }

    private String decodeJniName(String mangled) {
        StringBuilder sb = new StringBuilder();
        int i = 0;
        while (i < mangled.length()) {
            char c = mangled.charAt(i);
            if (c == '_') {
                if (i + 1 < mangled.length()) {
                    char next = mangled.charAt(i + 1);
                    if (next == '1') {
                        sb.append('_'); i += 2; continue;
                    } else if (next == '2') {
                        sb.append(';'); i += 2; continue;
                    } else if (next == '3') {
                        sb.append('['); i += 2; continue;
                    } else if (next == '0' && i + 5 < mangled.length()) {
                        // _0XXXX unicode escape
                        String hex = mangled.substring(i + 2, i + 6);
                        if (hex.matches("[0-9a-fA-F]{4}")) {
                            sb.append((char) Integer.parseInt(hex, 16));
                            i += 6; continue;
                        }
                    }
                }
                // Plain '_' is a package/class separator -> '.'
                sb.append('.');
                i++;
            } else {
                sb.append(c);
                i++;
            }
        }
        return sb.toString();
    }
}