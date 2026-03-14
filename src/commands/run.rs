use std::path::{PathBuf, Path};
use crate::apk::{Framework, GdxBackend};
use zip::ZipArchive;
use std::io::Read;

pub fn execute(apk: &PathBuf) -> anyhow::Result<()> {
    log::debug!("Executing APK: {}", apk.display());

    if !apk.exists() {
        anyhow::bail!("APK path does not exist or cannot be accessed.");
    }

    let framework = crate::apk::Framework::detect(&apk)?;

    match framework {
        Framework::LibGdx(backend) => {
            log::debug!("Executing APK with LibGdx framework.");
            libgdx_run(&apk, &backend)?
        }
        Framework::Unknown => {
            anyhow::bail!("Cannot work on unknown APK framework.")
        }
    }

    Ok(())
}

pub fn libgdx_run(apk: &Path, backend: &GdxBackend) -> anyhow::Result<()> {
    // Extract to temprorary file
    let tmp = tempfile::tempdir()?;
    crate::apk::extract(apk, tmp.path())?;

    log::debug!("Extraction complete. Extracted to: {}", tmp.path().display());

    // Convert dex to jar
    let classes_dex = tmp.path().join("classes.dex");
    let app_jar = tmp.path().join("application.jar");
    crate::apk::dex::dex_to_jar(&classes_dex, &app_jar)?;

    // Extracting LibGDX version for installation of dependencies
    let libgdx_version = extract_libgdx_version(&app_jar)?;
    log::info!("Detected LibGDX version: {}", libgdx_version);
    log::debug!("Gdx backend found: {}", match backend {
        GdxBackend::Lwjgl3 => "LWJGL3",
        GdxBackend::Lwjgl2 => "LWJGL2",
    });

    // Ensure LWJGL/LibGDX desktop jars are cached
    let lib_dir = crate::deps::ensure_libgdx_deps(&libgdx_version, &backend)?;
    
    // Find main app class
    let main_class = crate::apk::class::find_main_class(&app_jar)?;
    log::info!("Detected main class: {}", main_class);
    
    // Generate source
    crate::launcher::write_launcher(&tmp.path(), &backend)?;
    
    // Run from assets directory
    let assets_dir = tmp.path().join("assets");
    let working_dir = if assets_dir.exists() {
        assets_dir
    } else {
        tmp.path().to_path_buf()
    };

    crate::jvm::run(&working_dir, &lib_dir, &app_jar, tmp.path(), &main_class, "App")?;
    
    Ok(())
}

pub fn extract_libgdx_version(jar: &Path) -> anyhow::Result<String> {
    let file = std::fs::File::open(jar)?;
    let mut zip = ZipArchive::new(file)?;
    
    let mut version_class = zip.by_name("com/badlogic/gdx/Version.class")?;
    let mut bytes = Vec::new();
    version_class.read_to_end(&mut bytes)?;
    
    let content = String::from_utf8_lossy(&bytes);
    for word in content.split(|c: char| !c.is_ascii_graphic()) {
        let parts: Vec<&str> = word.split('.').collect();
        if parts.len() == 3 && parts.iter().all(|p| p.parse::<u32>().is_ok()) {
            return Ok(word.to_string());
        }
    }
    
    anyhow::bail!("Could not extract LibGDX version from jar")
}