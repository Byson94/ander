use std::path::Path;

const ANDROID_STUBS: &[u8] = include_bytes!("../java/out/android-stubs.jar");
const GDX_STUBS: &[u8] = include_bytes!("../java/out/gdx-stubs.jar");
const ANDER_BRIDGE: &[u8] = include_bytes!("../java/out/AnderBridge.class");
const ANDER_BRIDGE_SO: &[u8] = include_bytes!("../natives/libAnderBridge.so");
const LAUNCHER_LWJGL3: &[u8] = include_bytes!("../java/out/AnderLauncherLwjgl3.class");
const LAUNCHER_LWJGL2: &[u8] = include_bytes!("../java/out/AnderLauncherLwjgl2.class");

pub fn write_launcher(output_dir: &Path) -> anyhow::Result<()> {
    std::fs::write(output_dir.join("android-stubs.jar"), ANDROID_STUBS)?;
    std::fs::write(output_dir.join("gdx-stubs.jar"), GDX_STUBS)?;
    std::fs::write(output_dir.join("AnderBridge.class"), ANDER_BRIDGE)?;
    std::fs::write(output_dir.join("libAnderBridge.so"), ANDER_BRIDGE_SO)?;
    std::fs::write(output_dir.join("AnderLauncherLwjgl3.class"), LAUNCHER_LWJGL3)?;
    std::fs::write(output_dir.join("AnderLauncherLwjgl2.class"), LAUNCHER_LWJGL2)?;
    Ok(())
}