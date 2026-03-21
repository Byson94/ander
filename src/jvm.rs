use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::io::{BufRead, BufReader};
use crate::apk::GdxBackend;
use crate::deps::libgdx::LibGdxDeps;

// Hardcoded for testing, will come from ander's data dir later
const BIONIC_LAUNCHER: &str = "/home/byson94/Documents/Programming/ander-bionic/launcher";
const BIONIC_SYSROOT: &str  = "/home/byson94/Documents/Programming/ander-bionic/sysroot";

pub fn run(
    working_dir: &Path,
    deps: &LibGdxDeps,
    app_jar: &Path,
    launcher_dir: &Path,
    main_class: &str,
    app_name: &str,
) -> anyhow::Result<()> {
    let java = find_java()?;

    // Collect ARM64 .so files from APK's lib/arm64-v8a/
    let so_files = collect_arm64_libs(launcher_dir);
    if so_files.is_empty() {
        anyhow::bail!("No ARM64 .so files found in lib/arm64-v8a/");
    }
    log::info!("Found {} ARM64 libs", so_files.len());

    // Spawn the bionic launcher and wait for it to be ready
    let mut lib_launcher = spawn_lib_launcher(
        Path::new(BIONIC_LAUNCHER),
        Path::new(BIONIC_SYSROOT),
        &so_files,
    )?;

    // Wait for "listening on ..." line on stdout
    log::info!("Waiting for bionic launcher to be ready...");
    let stdout = lib_launcher.stdout.take()
        .ok_or_else(|| anyhow::anyhow!("Failed to get launcher stdout"))?;
    let mut reader = BufReader::new(stdout);

    // Wait for "listening"
    loop {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let line = line.trim_end().to_string();
        log::debug!("launcher: {}", line);
        if line.contains("listening on") {
            log::info!("Bionic launcher ready: {}", line);
            break;
        }
    }

    // Forward remaining stdout in background
    std::thread::spawn(move || {
        for line in reader.lines().flatten() {
            eprintln!("[launcher:out] {}", line);
        }
    });

    // Forward stderr
    if let Some(stderr) = lib_launcher.stderr.take() {
        std::thread::spawn(move || {
            let reader = BufReader::new(stderr);
            for line in reader.lines().flatten() {
                eprintln!("[launcher:err] {}", line);
            }
        });
    }

    // Detect backend
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
    let ander_bridge_path = launcher_dir.join("libAnderBridge.so")
        .to_string_lossy().to_string();

    log::info!("Launching JVM...");
    let status = Command::new(&java)
        .arg("--enable-native-access=ALL-UNNAMED")
        .arg(format!("-Dander.app.name={}", app_name))
        .arg(format!("-Dander.bridge.path={}", ander_bridge_path))
        .arg(format!("-Dander.app.jar={}", app_jar.display()))
        .arg("-cp")
        .arg(&classpath_str)
        .arg(launcher_class)
        .arg(main_class)
        .arg("480")
        .arg("800")
        .arg(app_name)
        .current_dir(working_dir)
        .status()?;

    // Clean up launcher process
    let _ = lib_launcher.kill();

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
        launcher_dir.join("gdx-stubs.jar").to_string_lossy().to_string(),
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

fn collect_arm64_libs(launcher_dir: &Path) -> Vec<PathBuf> {
    let arm64_dir = launcher_dir.join("lib").join("arm64-v8a");
    if !arm64_dir.exists() {
        return vec![];
    }
    std::fs::read_dir(&arm64_dir)
        .unwrap()
        .flatten()
        .map(|e| e.path())
        .filter(|p| p.extension().map(|e| e == "so").unwrap_or(false))
        .collect()
}

fn spawn_lib_launcher(
    launcher_bin: &Path,
    sysroot: &Path,
    so_files: &[PathBuf],
) -> anyhow::Result<std::process::Child> {
    let mut cmd = Command::new("qemu-aarch64-static");
    cmd.arg("-L").arg(sysroot);
    cmd.arg(launcher_bin);
    for so in so_files {
        cmd.arg(so);
    }
    cmd.stdout(Stdio::piped());
    cmd.stderr(Stdio::piped());
    let child = cmd.spawn()?;
    Ok(child)
}