use std::process::Command;
use std::path::Path;
use anyhow::anyhow;

pub fn dex_to_jar(dex_path: &Path, output_jar: &Path) -> anyhow::Result<()> {
    let status = Command::new("dex2jar")
        .arg(dex_path)
        .arg("-o")
        .arg(output_jar)
        .status()?;
    
    if !status.success() {
        return Err(anyhow!("dex2jar failed"));
    }
    Ok(())
}
