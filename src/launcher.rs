use std::path::Path;
use crate::apk::GdxBackend;

const LAUNCHER_LWJGL3: &[u8] = include_bytes!("../precompiled/AnderLauncherLwjgl3.class");
const LAUNCHER_LWJGL2: &[u8] = include_bytes!("../precompiled/AnderLauncherLwjgl2.class");

pub fn write_launcher(output_dir: &Path, backend: &GdxBackend) -> anyhow::Result<()> {
    let (bytes, name) = match backend {
        GdxBackend::Lwjgl3 => (LAUNCHER_LWJGL3, "AnderLauncherLwjgl3.class"),
        GdxBackend::Lwjgl2 => (LAUNCHER_LWJGL2, "AnderLauncherLwjgl2.class"),
    };
    std::fs::write(output_dir.join(name), bytes)?;
    Ok(())
}