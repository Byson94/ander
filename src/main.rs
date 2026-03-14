mod opts;
mod commands;
mod apk;
mod jvm;
mod launcher;
mod deps;

use clap::Parser;
use opts::{Cli, Commands};

fn main() {
    let cli = Cli::parse();

    // Setup debug level
    let log_level = if cli.debug {
        log::LevelFilter::Debug
    } else {
        log::LevelFilter::Info
    };

    pretty_env_logger::formatted_builder()
        .filter_level(log_level)
        .init();

    // Parse commands
    match &cli.commands {
        Commands::Run { apk } => {
            if let Err(e) = commands::run::execute(apk) {
                log::error!("Failed to run APK: {e}");
            }
        }
    }
}
