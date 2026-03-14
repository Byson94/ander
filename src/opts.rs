use clap::{Parser, Subcommand};
use std::path::PathBuf;

/// Android app runner for Linux
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
#[command(arg_required_else_help = true)]
pub struct Cli {
    #[arg(short, long, global = true)]
    pub debug: bool,

    #[command(subcommand)]
    pub commands: Commands,
}

#[derive(Subcommand, Debug)]
pub enum Commands {
    /// Run an APK file
    Run {
        /// Path to the APK to run
        apk: PathBuf,
    }
}