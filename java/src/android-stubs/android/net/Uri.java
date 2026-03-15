package android.net;

public class Uri {
    private final String uriString;
    private Uri(String uri) { this.uriString = uri; }

    public static Uri parse(String uriString) { return new Uri(uriString); }
    public static Uri fromFile(java.io.File file) { return new Uri(file.toURI().toString()); }

    public String getScheme()   { try { return new java.net.URI(uriString).getScheme();   } catch (Exception e) { return null; } }
    public String getHost()     { try { return new java.net.URI(uriString).getHost();     } catch (Exception e) { return null; } }
    public String getPath()     { try { return new java.net.URI(uriString).getPath();     } catch (Exception e) { return null; } }
    public String getQuery()    { try { return new java.net.URI(uriString).getQuery();    } catch (Exception e) { return null; } }
    public String getFragment() { try { return new java.net.URI(uriString).getFragment(); } catch (Exception e) { return null; } }

    public String getQueryParameter(String key) {
        String q = getQuery();
        if (q == null) return null;
        for (String part : q.split("&")) {
            String[] kv = part.split("=", 2);
            if (kv.length == 2 && kv[0].equals(key)) return kv[1];
        }
        return null;
    }

    @Override public String toString()        { return uriString; }
    @Override public boolean equals(Object o) { return o instanceof Uri && uriString.equals(((Uri) o).uriString); }
    @Override public int hashCode()           { return uriString.hashCode(); }
}