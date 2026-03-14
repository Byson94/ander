use std::{fs, path::Path};
use zip::ZipArchive;

pub fn extract(apk: &Path, dest: &Path) -> anyhow::Result<()> {
    let file = fs::File::open(apk)?;
    let mut archive = ZipArchive::new(file)?;
    archive.extract(dest)?;
    Ok(())
}