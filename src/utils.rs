use std::path::PathBuf;

pub fn get_app_data_dir(path: &str) -> anyhow::Result<PathBuf> {
    let base = dirs::data_local_dir()
        .ok_or_else(|| anyhow::anyhow!("Could not find local data directory"))?;
    Ok(base.join("ander").join("apps").join(path))
}