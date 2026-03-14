use std::path::{Path, PathBuf};
use std::process::Command;

pub fn run(
    working_dir: &Path,
    lib_dir: &Path,
    app_jar: &Path,
    launcher_dir: &Path,
    main_class: &str,
    app_name: &str,
) -> anyhow::Result<()> {
    let java = find_java()?;

    let mut classpath = vec![
        launcher_dir.to_string_lossy().to_string(),
        app_jar.to_string_lossy().to_string(),
    ];

    for entry in std::fs::read_dir(lib_dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.extension().map(|e| e == "jar").unwrap_or(false) {
            classpath.push(path.to_string_lossy().to_string());
        }
    }

    let classpath_str = classpath.join(":");

    log::info!("Launching DesktopLauncher...");

    let status = Command::new(&java)
        .arg("--enable-native-access=ALL-UNNAMED")
        .arg("-cp")
        .arg(&classpath_str)
        .arg("AnderLauncher")
        .arg(&main_class)
        .arg("480")
        .arg("800")
        .arg(&app_name)
        .current_dir(working_dir)
        .status()?;

    if !status.success() {
        anyhow::bail!("JVM exited with error: {}", status);
    }

    Ok(())
}

fn find_java() -> anyhow::Result<PathBuf> {
    if let Ok(java_home) = std::env::var("JAVA_HOME") {
        let java = PathBuf::from(java_home).join("bin/java");
        if java.exists() {
            return Ok(java);
        }
    }

    which::which("java").map_err(|_| anyhow::anyhow!(
        "Java runtime not found. Install with: sudo pacman -S jre-openjdk"
    ))
}