use std::path::{Path, PathBuf};
use crate::apk::GdxBackend;

pub fn ensure_libgdx_deps(version: &str, backend: &GdxBackend) -> anyhow::Result<PathBuf> {
    let cache_dir = get_cache_dir(version)?;
    if !cache_dir.exists() {
        std::fs::create_dir_all(&cache_dir)?;
        download_libgdx_deps(version, &cache_dir, backend)?;
    }
    Ok(cache_dir)
}

fn get_cache_dir(version: &str) -> anyhow::Result<PathBuf> {
    let base = dirs::data_local_dir()
        .ok_or_else(|| anyhow::anyhow!("Could not find local data directory"))?;
    Ok(base.join("ander").join("cache").join("libgdx").join(version))
}

fn download_libgdx_deps(version: &str, cache_dir: &Path, backend: &GdxBackend) -> anyhow::Result<()> {
    let mut jars = vec![
        // Core dependencies which are same for both backends
        format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-platform/{v}/gdx-platform-{v}-natives-desktop.jar", v = version),
        format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-freetype/{v}/gdx-freetype-{v}.jar", v = version),
        format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-freetype-platform/{v}/gdx-freetype-platform-{v}-natives-desktop.jar", v = version),
        "https://repo1.maven.org/maven2/com/badlogicgames/jlayer/jlayer/1.0.1-gdx/jlayer-1.0.1-gdx.jar".to_string(),
    ];

    match backend {
        GdxBackend::Lwjgl3 => {
            jars.push(format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-backend-lwjgl3/{v}/gdx-backend-lwjgl3-{v}.jar", v = version));
            for artifact in &["lwjgl", "lwjgl-glfw", "lwjgl-opengl", "lwjgl-openal"] {
                jars.push(lwjgl_url(artifact, version));
                jars.push(lwjgl_natives_url(artifact, version));
            }
        }
        GdxBackend::Lwjgl2 => {
            jars.push(format!("https://repo1.maven.org/maven2/com/badlogicgames/gdx/gdx-backend-lwjgl/{v}/gdx-backend-lwjgl-{v}.jar", v = version));
            jars.push("https://repo1.maven.org/maven2/org/lwjgl/lwjgl/lwjgl-platform/2.9.3/lwjgl-platform-2.9.3-natives-linux.jar".to_string());
            jars.push("https://repo1.maven.org/maven2/org/lwjgl/lwjgl/lwjgl/2.9.3/lwjgl-2.9.3.jar".to_string());
        }
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