use std::path::Path;

const LAUNCHER_LWJGL3: &[u8] = include_bytes!("../java/out/AnderLauncherLwjgl3.class");
const LAUNCHER_LWJGL2: &[u8] = include_bytes!("../java/out/AnderLauncherLwjgl2.class");

pub fn write_launcher(output_dir: &Path) -> anyhow::Result<()> {
    std::fs::write(output_dir.join("AnderLauncherLwjgl3.class"), LAUNCHER_LWJGL3)?;
    std::fs::write(output_dir.join("AnderLauncherLwjgl2.class"), LAUNCHER_LWJGL2)?;
    Ok(())
}