#!/usr/bin/env sh

RAYLIB_VERSION=5.0

wget -P dep https://github.com/raysan5/raylib/releases/download/${RAYLIB_VERSION}/raylib-${RAYLIB_VERSION}_linux_amd64.tar.gz
tar -xf dep/raylib-${RAYLIB_VERSION}_linux_amd64.tar.gz -C dep
mv dep/raylib-${RAYLIB_VERSION}_linux_amd64 dep/raylib
