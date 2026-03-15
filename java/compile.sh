#!/bin/bash
set -e
GDX_VERSION="1.9.10"
LWJGL2_GDX_VERSION="1.9.10"

mkdir -p libs-lwjgl3 libs-lwjgl2
BASE="https://repo1.maven.org/maven2/com/badlogicgames/gdx"

# LWJGL3 jars
wget -nc -P libs-lwjgl3 "${BASE}/gdx/${GDX_VERSION}/gdx-${GDX_VERSION}.jar"
wget -nc -P libs-lwjgl3 "${BASE}/gdx-backend-lwjgl3/${GDX_VERSION}/gdx-backend-lwjgl3-${GDX_VERSION}.jar"

# LWJGL2 jars
wget -nc -P libs-lwjgl2 "${BASE}/gdx/${LWJGL2_GDX_VERSION}/gdx-${LWJGL2_GDX_VERSION}.jar"
wget -nc -P libs-lwjgl2 "${BASE}/gdx-backend-lwjgl/${LWJGL2_GDX_VERSION}/gdx-backend-lwjgl-${LWJGL2_GDX_VERSION}.jar"

# Compile
STUBS_TMP=$(mktemp -d)
trap 'rm -rf "$STUBS_TMP"' EXIT
javac -source 8 -target 8 \
    -d "$STUBS_TMP" \
    $(find src/android-stubs -name "*.java")
jar cf out/android-stubs.jar -C "$STUBS_TMP" .
echo "Built android-stubs.jar"

javac -cp "libs-lwjgl3/gdx-${GDX_VERSION}.jar:libs-lwjgl3/gdx-backend-lwjgl3-${GDX_VERSION}.jar" \
    -d out src/AnderLauncherLwjgl3.java
javac -cp "libs-lwjgl2/gdx-${GDX_VERSION}.jar:libs-lwjgl2/gdx-backend-lwjgl-${LWJGL2_GDX_VERSION}.jar" \
    -d out src/AnderLauncherLwjgl2.java
echo "Built launchers for both backends: Lwjgl2 and Lwjgl3"

echo "Done. Commit out/ directory."