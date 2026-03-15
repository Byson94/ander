use std::path::{Path, PathBuf};

pub struct LibGdxDeps {
    pub lwjgl3_dir: PathBuf,
    pub lwjgl2_dir: PathBuf,
}

pub fn ensure_libgdx_deps(version: &str) -> anyhow::Result<LibGdxDeps> {
    let lwjgl3_dir = get_cache_dir(&format!("{}/lwjgl3", version))?;
    let lwjgl2_dir = get_cache_dir(&format!("{}/lwjgl2", version))?;
    std::fs::create_dir_all(&lwjgl3_dir)?;
    std::fs::create_dir_all(&lwjgl2_dir)?;
    download_libgdx_deps(version, &lwjgl3_dir, true)?;
    download_libgdx_deps(version, &lwjgl2_dir, false)?;
    Ok(LibGdxDeps { lwjgl3_dir, lwjgl2_dir })
}

fn get_cache_dir(path: &str) -> anyhow::Result<PathBuf> {
    let base = dirs::data_local_dir()
        .ok_or_else(|| anyhow::anyhow!("Could not find local data directory"))?;
    Ok(base.join("ander").join("cache").join("libgdx").join(path))
}

fn download_libgdx_deps(version: &str, cache_dir: &Path, lwjgl3: bool) -> anyhow::Result<()> {
    // Dependencies that both will share.
    let mut jars = vec![
        // Core libgdx stuff
        format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-platform/{v}/gdx-platform-{v}-natives-desktop.jar", v = version),
        format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-freetype/{v}/gdx-freetype-{v}.jar", v = version),
        format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-freetype-platform/{v}/gdx-freetype-platform-{v}-natives-desktop.jar", v = version),
        // JLayer: MP3 audio decoding (gdx fork)
        "https://repo1.maven.org/maven2/com/badlogicgames/jlayer/jlayer/1.0.1-gdx/jlayer-1.0.1-gdx.jar".to_string(),
        // xerial sqlite-jdbc: SQLite driver (replaces Android's built-in SQLite)
        "https://repo1.maven.org/maven2/org/xerial/sqlite-jdbc/3.45.3.0/sqlite-jdbc-3.45.3.0.jar".to_string(),
        // slf4j-api: logging facade (required by sqlite-jdbc)
        "https://repo1.maven.org/maven2/org/slf4j/slf4j-api/1.7.36/slf4j-api-1.7.36.jar".to_string(),
        // slf4j-simple: minimal slf4j binding, prints to stdout
        "https://repo1.maven.org/maven2/org/slf4j/slf4j-simple/1.7.36/slf4j-simple-1.7.36.jar".to_string(),
        // org.json: JSON parsing
        "https://repo1.maven.org/maven2/org/json/json/20240303/json-20240303.jar".to_string(),
    ];

    if lwjgl3 {
        jars.push(format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-backend-lwjgl3/{v}/gdx-backend-lwjgl3-{v}.jar", v = version));
        for artifact in &["lwjgl", "lwjgl-glfw", "lwjgl-opengl", "lwjgl-openal"] {
            jars.push(lwjgl_url(artifact, version));
            jars.push(lwjgl_natives_url(artifact, version));
        }
    } else {
        jars.push(format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-backend-lwjgl/{v}/gdx-backend-lwjgl-{v}.jar", v = version));
        jars.push("https://repo1.maven.org/maven2/org/lwjgl/lwjgl/lwjgl-platform/2.9.3/lwjgl-platform-2.9.3-natives-linux.jar".to_string());
        jars.push("https://repo1.maven.org/maven2/org/lwjgl/lwjgl/lwjgl/2.9.3/lwjgl-2.9.3.jar".to_string());
    }

    for url in &jars {
        let filename = url.split('/').last()
            .ok_or_else(|| anyhow::anyhow!("Invalid URL: {}", url))?;
        let dest = cache_dir.join(filename);
        if dest.exists() {
            continue;
        }
        log::info!("Downloading {}...", filename);
        if let Err(e) = download_file(url, &dest) {
            log::warn!("Skipping {}: {}", filename, e);
        }
    }

    Ok(())
}

fn lwjgl_version_for_gdx(gdx_version: &str) -> &'static str {
    let minor: u32 = gdx_version.split('.')
        .nth(1)
        .and_then(|v| v.parse().ok())
        .unwrap_or(9);
    if minor >= 10 { "3.3.1" } else { "3.2.3" }
}

fn lwjgl_url(artifact: &str, gdx_version: &str) -> String {
    let v = lwjgl_version_for_gdx(gdx_version);
    format!("https://repo1.maven.org/maven2/org/lwjgl/{a}/{v}/{a}-{v}.jar", a = artifact, v = v)
}

fn lwjgl_natives_url(artifact: &str, gdx_version: &str) -> String {
    let v = lwjgl_version_for_gdx(gdx_version);
    format!("https://repo1.maven.org/maven2/org/lwjgl/{a}/{v}/{a}-{v}-natives-linux.jar", a = artifact, v = v)
}

pub fn download_file(url: &str, dest: &Path) -> anyhow::Result<()> {
    let response = ureq::get(url).call()
        .map_err(|e| anyhow::anyhow!("Failed to download {}: {}", url, e))?;
    let mut file = std::fs::File::create(dest)?;
    std::io::copy(&mut response.into_body().as_reader(), &mut file)?;
    Ok(())
}