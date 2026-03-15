use std::process::Command;
use std::path::{Path, PathBuf};
use anyhow::anyhow;

pub fn dex_to_jar(tmp_dir: &Path, output_jar: &Path) -> anyhow::Result<()> {
    // Find all classes*.dex files
    let mut dex_files: Vec<PathBuf> = std::fs::read_dir(tmp_dir)?
        .filter_map(|e| e.ok())
        .map(|e| e.path())
        .filter(|p| {
            p.file_name()
                .and_then(|n| n.to_str())
                .map(|n| n.starts_with("classes") && n.ends_with(".dex"))
                .unwrap_or(false)
        })
        .collect();

    dex_files.sort();

    let mut temp_jars = Vec::new();
    for dex in &dex_files {
        let temp_jar = dex.with_extension("jar");
        let status = Command::new("dex2jar")
            .arg(dex)
            .arg("-o")
            .arg(&temp_jar)
            .arg("--force")
            .status()?;
        if status.success() {
            temp_jars.push(temp_jar);
        } else {
            log::warn!("dex2jar failed for {}", dex.display());
        }
    }

    if temp_jars.is_empty() {
        return Err(anyhow!("dex2jar failed for all dex files"));
    }

    merge_jars(&temp_jars, output_jar)?;

    for jar in &temp_jars {
        std::fs::remove_file(jar).ok();
    }

    Ok(())
}

fn merge_jars(jars: &[PathBuf], output: &Path) -> anyhow::Result<()> {
    let out_file = std::fs::File::create(output)?;
    let mut out_zip = zip::ZipWriter::new(out_file);
    let options = zip::write::SimpleFileOptions::default();
    let mut seen = std::collections::HashSet::new();

    for jar in jars {
        let in_file = std::fs::File::open(jar)?;
        let mut in_zip = zip::ZipArchive::new(in_file)?;
        for i in 0..in_zip.len() {
            let mut entry = in_zip.by_index(i)?;
            let name = entry.name().to_string();
            if seen.contains(&name) { continue; }
            seen.insert(name.clone());
            out_zip.start_file(&name, options)?;
            std::io::copy(&mut entry, &mut out_zip)?;
        }
    }

    out_zip.finish()?;
    Ok(())
}