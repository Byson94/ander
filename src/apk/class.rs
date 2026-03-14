use std::path::Path;
use zip::ZipArchive;
use std::io::Read;

pub fn find_main_class(jar: &Path) -> anyhow::Result<String> {
    let file = std::fs::File::open(jar)?;
    let mut zip = ZipArchive::new(file)?;

    for i in 0..zip.len() {
        let mut entry = zip.by_index(i)?;
        let name = entry.name().to_string();

        if !name.ends_with("AndroidLauncher.class") {
            continue;
        }

        let mut bytes = Vec::new();
        entry.read_to_end(&mut bytes)?;
        let content = String::from_utf8_lossy(&bytes);

        let package = name
            .trim_end_matches("AndroidLauncher.class")
            .trim_end_matches('/');

        for word in content.split(|c: char| !c.is_ascii_graphic()) {
            if word.starts_with(package)
                && !word.ends_with("AndroidLauncher")
                && !word.contains('$')
                && word.contains('/') {
                return Ok(word.replace('/', "."));
            }
        }
    }

    anyhow::bail!("Could not find main class from AndroidLauncher")
}