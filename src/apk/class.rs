use std::path::Path;
use std::io::Read;
use zip::ZipArchive;

pub fn find_main_class(jar: &Path) -> anyhow::Result<String> {
    let file = std::fs::File::open(jar)?;
    let mut zip = ZipArchive::new(file)?;

    // Try MANIFEST.MF first
    if let Ok(manifest_class) = read_manifest_main_class(&mut zip) {
        return Ok(manifest_class);
    }

    // If not found in MANIFEST.MF, try known classes
    let launcher_suffixes = [
        "AndroidLauncher.class",
        "DesktopLauncher.class",
        "Lwjgl3Launcher.class",
        "Launcher.class",
    ];

    for suffix in &launcher_suffixes {
        if let Ok(class) = scan_launcher_class(&mut zip, suffix) {
            return Ok(class);
        }
    }

    anyhow::bail!("Could not find main class in {}", jar.display())
}

fn read_manifest_main_class(zip: &mut ZipArchive<std::fs::File>) -> anyhow::Result<String> {
    let mut entry = zip.by_name("META-INF/MANIFEST.MF")?;
    let mut contents = String::new();
    entry.read_to_string(&mut contents)?;

    for line in contents.lines() {
        if let Some(value) = line.strip_prefix("Main-Class:") {
            let class = value.trim().to_string();
            if !class.is_empty() {
                return Ok(class);
            }
        }
    }
    anyhow::bail!("No Main-Class in manifest")
}

fn scan_launcher_class(
    zip: &mut ZipArchive<std::fs::File>,
    suffix: &str,
) -> anyhow::Result<String> {
    let entry_name = (0..zip.len())
        .find_map(|i| {
            let entry = zip.by_index(i).ok()?;
            let name = entry.name().to_string();
            if name.ends_with(suffix) { Some(name) } else { None }
        })
        .ok_or_else(|| anyhow::anyhow!("No {suffix} found"))?;

    let package = entry_name
        .strip_suffix(suffix)
        .unwrap_or("")
        .trim_end_matches('/');

    let mut entry = zip.by_name(&entry_name)?;
    let mut bytes = Vec::new();
    entry.read_to_end(&mut bytes)?;

    extract_class_name_from_bytecode(&bytes, package)
}

fn extract_class_name_from_bytecode(bytes: &[u8], package: &str) -> anyhow::Result<String> {
    let strings = parse_constant_pool_strings(bytes)?;

    for s in &strings {
        if s.starts_with(package)
            && !s.ends_with("AndroidLauncher")
            && !s.ends_with("DesktopLauncher")
            && !s.ends_with("Lwjgl3Launcher")
            && !s.contains('$')
            && s.contains('/')
        {
            return Ok(s.replace('/', "."));
        }
    }
    anyhow::bail!("No candidate class found in constant pool")
}

// Minimal constant pool parser.
// See: https://docs.oracle.com/javase/specs/jvms/se21/html/jvms-4.html
fn parse_constant_pool_strings(bytes: &[u8]) -> anyhow::Result<Vec<String>> {
    if bytes.len() < 10 || &bytes[0..4] != b"\xCA\xFE\xBA\xBE" {
        anyhow::bail!("Not a valid class file");
    }

    let pool_count = u16::from_be_bytes([bytes[8], bytes[9]]) as usize;
    let mut pos = 10;
    let mut strings = Vec::new();

    let mut i = 1;
    while i < pool_count {
        if pos >= bytes.len() {
            break;
        }
        let tag = bytes[pos];
        pos += 1;
        match tag {
            1 => {
                // Utf8
                if pos + 2 > bytes.len() { break; }
                let len = u16::from_be_bytes([bytes[pos], bytes[pos + 1]]) as usize;
                pos += 2;
                if pos + len > bytes.len() { break; }
                if let Ok(s) = std::str::from_utf8(&bytes[pos..pos + len]) {
                    strings.push(s.to_string());
                }
                pos += len;
            }
            7 | 8 | 16 | 19 | 20 => pos += 2, // Class, String, MethodType, Module, Package
            9 | 10 | 11 | 3 | 4 | 12 | 17 | 18 => pos += 4, // Fieldref, Methodref, etc.
            5 | 6 => { pos += 8; i += 1; } // Long, Double take two slots
            15 => pos += 3, // MethodHandle
            _ => anyhow::bail!("Unknown constant pool tag {tag} at pos {pos}"),
        }
        i += 1;
    }

    Ok(strings)
}