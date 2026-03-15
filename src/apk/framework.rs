use std::path::Path;
use zip::ZipArchive;

pub enum GdxBackend {
    Lwjgl2,
    Lwjgl3,
}

pub enum Framework {
    LibGdx,
    Unknown,
}

impl Framework {
    pub fn detect(apk: &Path) -> anyhow::Result<Self> {
        let file = std::fs::File::open(apk)?;
        let mut zip = ZipArchive::new(file)?;

        let is_libgdx = (0..zip.len()).any(|i| {
            zip.by_index(i)
                .map(|f| f.name().contains("libgdx"))
                .unwrap_or(false)
        });

        if is_libgdx {
            return Ok(Self::LibGdx);
        }

        Ok(Self::Unknown)
    }
}