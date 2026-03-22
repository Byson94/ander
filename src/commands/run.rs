use std::path::{PathBuf, Path};
use crate::apk::Framework;
use zip::ZipArchive;
use std::io::Read;

pub fn execute(apk: &PathBuf) -> anyhow::Result<()> {
    log::debug!("Executing APK: {}", apk.display());

    if !apk.exists() {
        anyhow::bail!("APK path does not exist or cannot be accessed.");
    }

    let framework = crate::apk::Framework::detect(&apk)?;

    match framework {
        Framework::LibGdx => {
            log::debug!("Executing APK with LibGdx framework.");
            libgdx_run(&apk)?
        }
        Framework::Unknown => {
            anyhow::bail!("Cannot work on unknown APK framework.")
        }
    }

    Ok(())
}

pub fn libgdx_run(apk: &Path) -> anyhow::Result<()> {
    // Extract to temprorary file
    let tmp = tempfile::tempdir()?;
    crate::apk::extract(apk, tmp.path())?;

    log::debug!("Extraction complete. Extracted to: {}", tmp.path().display());

    // Convert dex to jar
    let app_jar = tmp.path().join("application.jar");
    crate::apk::dex::dex_to_jar(&tmp.path(), &app_jar)?;

    // Extracting LibGDX version for installation of dependencies
    let libgdx_version = extract_libgdx_version(&app_jar)?;
    log::info!("Detected LibGDX version: {}", libgdx_version);

    // Extracting application name
    let app_name = get_package_name(tmp.path())?;
    log::info!("Extracted package name: {}", app_name);

    // Ensure LWJGL/LibGDX desktop jars are cached
    let lib_dir = crate::deps::ensure_libgdx_deps(&libgdx_version)?;
    
    // Find main app class
    let main_class = crate::apk::class::find_main_class(&app_jar)?;
    log::info!("Detected main class: {}", main_class);
    
    // Generate source
    crate::launcher::write_launcher(&tmp.path())?;
    
    // Run from assets directory
    let assets_dir = tmp.path().join("assets");
    let working_dir = if assets_dir.exists() {
        assets_dir
    } else {
        log::warn!("Failed to find assets directory.");
        tmp.path().to_path_buf()
    };

    crate::jvm::run(&working_dir, &lib_dir, &app_jar, tmp.path(), &main_class, &app_name)?;
    
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

pub fn get_package_name(extracted_dir: &Path) -> anyhow::Result<String> {
    let bytes = std::fs::read(extracted_dir.join("AndroidManifest.xml"))?;
    let words: Vec<u16> = bytes.chunks_exact(2)
        .map(|c| u16::from_le_bytes([c[0], c[1]]))
        .collect();
    let mut i = 0;
    while i < words.len() {
        let len = words[i] as usize;
        if len > 2 && len < 100 && i + len < words.len() {
            let s: String = char::decode_utf16(words[i+1..i+1+len].iter().copied())
                .filter_map(|r| r.ok())
                .collect();
            let parts: Vec<&str> = s.split('.').collect();
            if parts.len() >= 2
                && parts.iter().all(|p| !p.is_empty()
                    && p.chars().all(|c| c.is_ascii_alphanumeric() || c == '_'))
                && parts[0].starts_with(|c: char| c.is_ascii_alphabetic())
                && parts.iter().all(|p| p.starts_with(|c: char| c.is_ascii_alphabetic()))
            {
                return Ok(s);
            }
        }
        i += 1;
    }
    anyhow::bail!("Could not extract package name from AndroidManifest.xml")
}