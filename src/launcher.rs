use std::path::Path;

pub fn write_launcher(output_dir: &Path) -> anyhow::Result<()> {
    const LAUNCHER_CLASS: &[u8] = include_bytes!("../precompiled/AnderLauncher.class");
    std::fs::write(output_dir.join("AnderLauncher.class"), LAUNCHER_CLASS)?;
    Ok(())
}