use std::path::{Path, PathBuf};
use std::process::Command;
use crate::apk::GdxBackend;
use crate::deps::libgdx::LibGdxDeps;

pub fn run(
    working_dir: &Path,
    deps: &LibGdxDeps,
    app_jar: &Path,
    launcher_dir: &Path,
    main_class: &str,
    app_name: &str,
) -> anyhow::Result<()> {
    let java = find_java()?;

    let backend = if dry_run(&java, &deps.lwjgl3_dir, app_jar, launcher_dir)? {
        log::debug!("Dry run: LWJGL3 works");
        GdxBackend::Lwjgl3
    } else if dry_run(&java, &deps.lwjgl2_dir, app_jar, launcher_dir)? {
        log::debug!("Dry run: LWJGL2 works");
        GdxBackend::Lwjgl2
    } else {
        anyhow::bail!("Neither LWJGL3 nor LWJGL2 backend could be loaded");
    };

    let (lib_dir, launcher_class) = match backend {
        GdxBackend::Lwjgl3 => (&deps.lwjgl3_dir, "AnderLauncherLwjgl3"),
        GdxBackend::Lwjgl2 => (&deps.lwjgl2_dir, "AnderLauncherLwjgl2"),
    };

    let classpath_str = build_classpath(launcher_dir, app_jar, lib_dir);

    log::info!("Launching DesktopLauncher...");
    let status = Command::new(&java)
        .arg("--enable-native-access=ALL-UNNAMED")
        .arg(format!("-Dander.app.name={}", app_name))
        .arg("-cp")
        .arg(&classpath_str)
        .arg(launcher_class)
        .arg(main_class)
        .arg("480")
        .arg("800")
        .arg(app_name)
        .current_dir(working_dir)
        .status()?;

    if !status.success() {
        anyhow::bail!("JVM exited with error: {}", status);
    }
    Ok(())
}

fn dry_run(java: &Path, lib_dir: &Path, app_jar: &Path, launcher_dir: &Path) -> anyhow::Result<bool> {
    let launcher_class = if lib_dir.to_string_lossy().contains("lwjgl3") {
        "AnderLauncherLwjgl3"
    } else {
        "AnderLauncherLwjgl2"
    };

    let classpath_str = build_classpath(launcher_dir, app_jar, lib_dir);

    let status = Command::new(java)
        .arg("-cp")
        .arg(&classpath_str)
        .arg(launcher_class)
        .arg("--dry-run")
        .status()?;

    Ok(status.success())
}

fn build_classpath(launcher_dir: &Path, app_jar: &Path, lib_dir: &Path) -> String {
    let mut classpath = vec![
        launcher_dir.to_string_lossy().to_string(),
        app_jar.to_string_lossy().to_string(),
        launcher_dir.join("android-stubs.jar").to_string_lossy().to_string(),
    ];
    if let Ok(entries) = std::fs::read_dir(lib_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().map(|e| e == "jar").unwrap_or(false) {
                classpath.push(path.to_string_lossy().to_string());
            }
        }
    }
    classpath.join(":")
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