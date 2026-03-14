use std::path::Path;
use zip::ZipArchive;

pub enum GdxBackend {
    Lwjgl2,
    Lwjgl3,
}

pub enum Framework {
    LibGdx(GdxBackend),
    Unknown,
}

impl Framework {
    pub fn detect(apk: &Path) -> anyhow::Result<Self> {
        let file = std::fs::File::open(apk)?;
        let mut zip = ZipArchive::new(file)?;

        let mut is_libgdx = false;
        let mut backend = None;

        for i in 0..zip.len() {
            let entry = zip.by_index(i)?;
            let name = entry.name();

            if name.contains("libgdx") || name.starts_with("com/badlogic/gdx") {
                is_libgdx = true;
            }
            if name.starts_with("com/badlogic/gdx/backends/lwjgl3") {
                backend = Some(GdxBackend::Lwjgl3);
            } else if name.starts_with("com/badlogic/gdx/backends/lwjgl/") {
                backend = Some(GdxBackend::Lwjgl2);
            }

            if is_libgdx && backend.is_some() {
                break;
            }
        }

        if is_libgdx {
            return Ok(Self::LibGdx(backend.unwrap_or(GdxBackend::Lwjgl3)));
        }

        Ok(Self::Unknown)
    }
}