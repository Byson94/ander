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
javac -cp "libs-lwjgl3/gdx-${GDX_VERSION}.jar:libs-lwjgl3/gdx-backend-lwjgl3-${GDX_VERSION}.jar" AnderLauncherLwjgl3.java
javac -cp "libs-lwjgl2/gdx-${LWJGL2_GDX_VERSION}.jar:libs-lwjgl2/gdx-backend-lwjgl-${LWJGL2_GDX_VERSION}.jar" AnderLauncherLwjgl2.java

echo "Done. Commit AnderLauncherLwjgl2.class and AnderLauncherLwjgl3.class"