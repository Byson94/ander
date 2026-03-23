use std::path::{PathBuf, Path};
use std::fs;
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

    // Extracting application name
    let app_name = get_package_name(tmp.path())?;
    log::info!("Extracted package name: {}", app_name);

    // Convert dex to jar
    let apps_dir = crate::utils::get_app_data_dir(&app_name)?;
    let app_jar = tmp.path().join("application.jar");

    if !apps_dir.is_dir() {
        crate::apk::dex::dex_to_jar(&tmp.path(), &app_jar)?;

        fs::create_dir_all(&apps_dir)?;
        fs::copy(&app_jar, apps_dir.join("application.jar"))?;
        log::info!("Cached generated application.jar to {}", apps_dir.display());
    } else {
        log::info!("Using application.jar cached in {}", apps_dir.display());
        fs::copy(apps_dir.join("application.jar"), &app_jar)?;
    }

    // Extracting LibGDX version for installation of dependencies
    let libgdx_version = extract_libgdx_version(&app_jar)?;
    log::info!("Detected LibGDX version: {}", libgdx_version);

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
    let manifest = axmldecoder::parse(&bytes)?;

    let root = manifest
        .get_root()
        .as_ref()
        .ok_or_else(|| anyhow::anyhow!("AndroidManifest.xml has no root element"))?;

    if let axmldecoder::Node::Element(element) = root {
        if let Some(package) = element.get_attributes().get("package") {
            return Ok(package.clone());
        }
    }

    anyhow::bail!("No package attribute found in AndroidManifest.xml")
}