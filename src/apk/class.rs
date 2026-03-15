use std::collections::HashMap;
use std::path::Path;
use std::io::Read;
use zip::ZipArchive;

pub fn find_main_class(jar: &Path) -> anyhow::Result<String> {
    let file = std::fs::File::open(jar)?;
    let mut zip = ZipArchive::new(file)?;

    if let Ok(manifest_class) = read_manifest_main_class(&mut zip) {
        return Ok(manifest_class);
    }

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

struct ClassInfo {
    superclass: Option<String>,
}

fn parse_class_info(bytes: &[u8]) -> anyhow::Result<ClassInfo> {
    if bytes.len() < 10 || &bytes[0..4] != b"\xCA\xFE\xBA\xBE" {
        anyhow::bail!("Not a valid class file");
    }

    let pool_count = u16::from_be_bytes([bytes[8], bytes[9]]) as usize;
    let mut pos = 10;
    let mut utf8_map: HashMap<usize, String> = HashMap::new();
    let mut class_map: HashMap<usize, usize> = HashMap::new();
    let mut i = 1usize;

    while i < pool_count {
        if pos >= bytes.len() { break; }
        let tag = bytes[pos]; pos += 1;
        match tag {
            1 => {
                if pos + 2 > bytes.len() { break; }
                let len = u16::from_be_bytes([bytes[pos], bytes[pos+1]]) as usize; pos += 2;
                if pos + len > bytes.len() { break; }
                if let Ok(s) = std::str::from_utf8(&bytes[pos..pos+len]) {
                    utf8_map.insert(i, s.to_string());
                }
                pos += len;
            }
            7 => {
                if pos + 2 > bytes.len() { break; }
                let name_idx = u16::from_be_bytes([bytes[pos], bytes[pos+1]]) as usize; pos += 2;
                class_map.insert(i, name_idx);
            }
            8 | 16 | 19 | 20 => pos += 2,
            9 | 10 | 11 | 3 | 4 | 12 | 17 | 18 => pos += 4,
            5 | 6 => { pos += 8; i += 1; }
            15 => pos += 3,
            _ => anyhow::bail!("Unknown constant pool tag {tag} at pos {pos}"),
        }
        i += 1;
    }

    // After constant pool: access_flags (2), this_class (2), super_class (2)
    if pos + 6 > bytes.len() {
        return Ok(ClassInfo { superclass: None });
    }
    // skip access_flags
    pos += 2;
    // skip this_class
    pos += 2;
    // read super_class index
    let super_class_idx = u16::from_be_bytes([bytes[pos], bytes[pos+1]]) as usize;

    let superclass = class_map.get(&super_class_idx)
        .and_then(|utf8_idx| utf8_map.get(utf8_idx))
        .cloned();

    Ok(ClassInfo { superclass })
}

fn is_gdx_game_class(class_name: &str, zip: &mut ZipArchive<std::fs::File>) -> bool {
    let mut current = class_name.to_string();
    for _ in 0..10 {
        if current == "com/badlogic/gdx/Game"
            || current == "com/badlogic/gdx/ApplicationListener" {
            return true;
        }
        // Stop at java/android/kotlin roots
        if current.starts_with("java/")
            || current.starts_with("android/")
            || current.starts_with("kotlin/") {
            return false;
        }

        let bytes = {
            let Ok(mut entry) = zip.by_name(&format!("{}.class", current)) else { return false; };
            let mut bytes = Vec::new();
            entry.read_to_end(&mut bytes).ok();
            bytes
        };

        match parse_class_info(&bytes) {
            Ok(info) => match info.superclass {
                Some(s) => current = s,
                None => return false,
            },
            Err(_) => return false,
        }
    }
    false
}

fn scan_launcher_class(
    zip: &mut ZipArchive<std::fs::File>,
    suffix: &str,
) -> anyhow::Result<String> {
    let all_classes: std::collections::HashSet<String> = (0..zip.len())
        .filter_map(|i| {
            let entry = zip.by_index(i).ok()?;
            let name = entry.name().to_string();
            name.ends_with(".class").then(|| name.trim_end_matches(".class").to_string())
        })
        .collect();

    let entry_name = (0..zip.len())
        .find_map(|i| {
            let entry = zip.by_index(i).ok()?;
            let name = entry.name().to_string();
            if name.ends_with(suffix) { Some(name) } else { None }
        })
        .ok_or_else(|| anyhow::anyhow!("No {suffix} found"))?;

    let bytes = {
        let mut entry = zip.by_name(&entry_name)?;
        let mut bytes = Vec::new();
        entry.read_to_end(&mut bytes)?;
        bytes
    };

    let strings = parse_constant_pool_strings(&bytes)?;

    for s in &strings {
        if s.contains('$') || s.contains('(') || s.contains(';') || !s.contains('/') {
            continue;
        }
        if all_classes.contains(s.as_str())
            && !s.starts_with("com/badlogic/gdx")
            && !s.starts_with("android")
            && !s.starts_with("java")
            && !s.ends_with("AndroidLauncher")
            && !s.ends_with("DesktopLauncher")
            && !s.ends_with("Lwjgl3Launcher")
        {
            let is_gdx = is_gdx_game_class(s, zip);
            log::debug!("candidate: {} is_gdx={}", s, is_gdx);
            if is_gdx {
                return Ok(s.replace('/', "."));
            }
        }
    }

    anyhow::bail!("No candidate class found in constant pool")
}

fn parse_constant_pool_strings(bytes: &[u8]) -> anyhow::Result<Vec<String>> {
    if bytes.len() < 10 || &bytes[0..4] != b"\xCA\xFE\xBA\xBE" {
        anyhow::bail!("Not a valid class file");
    }

    let pool_count = u16::from_be_bytes([bytes[8], bytes[9]]) as usize;
    let mut pos = 10;
    let mut strings = Vec::new();
    let mut i = 1;

    while i < pool_count {
        if pos >= bytes.len() { break; }
        let tag = bytes[pos]; pos += 1;
        match tag {
            1 => {
                if pos + 2 > bytes.len() { break; }
                let len = u16::from_be_bytes([bytes[pos], bytes[pos+1]]) as usize; pos += 2;
                if pos + len > bytes.len() { break; }
                if let Ok(s) = std::str::from_utf8(&bytes[pos..pos+len]) {
                    strings.push(s.to_string());
                }
                pos += len;
            }
            7 | 8 | 16 | 19 | 20 => pos += 2,
            9 | 10 | 11 | 3 | 4 | 12 | 17 | 18 => pos += 4,
            5 | 6 => { pos += 8; i += 1; }
            15 => pos += 3,
            _ => anyhow::bail!("Unknown constant pool tag {tag} at pos {pos}"),
        }
        i += 1;
    }

    Ok(strings)
}