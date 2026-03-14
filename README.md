# Ander

Android app execution program for Linux.

## How it works

Ander currently only supports running LibGdx based applications. 
It works by converting the dex into jar, extracting necessary information from it,
and then using a custom launcher to start the jar file with necessary dependencies.

Ander is currently under development and is not yet well tested, but a simple game like
[1010! Klooni](https://f-droid.org/en/packages/dev.lonami.klooni/) is found to work well.

## Depndencies

Ander currently requires two dependencies which are.

- `jre21-openjdk`: For rendering the window
- `dex2jar` (AUR): For converting dex into jar