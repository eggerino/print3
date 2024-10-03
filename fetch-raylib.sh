#!/usr/bin/env sh

# Specify version of raylib to fetch
RAYLIB_VERSION=5.0

# Download the tarball of the compiled raylib library
wget -P dep https://github.com/raysan5/raylib/releases/download/${RAYLIB_VERSION}/raylib-${RAYLIB_VERSION}_linux_amd64.tar.gz

# Extract the tarball
tar -xf dep/raylib-${RAYLIB_VERSION}_linux_amd64.tar.gz -C dep

# Rename the extracted directory
mv dep/raylib-${RAYLIB_VERSION}_linux_amd64 dep/raylib
