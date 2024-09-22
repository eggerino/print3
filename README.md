# print3

> Visualization Tool for 3D Models

![3D visualization of a bunny](/assets/bunny.png)

It supports the following file formats:
- .stl (binary and ascii)

It is also possible to describe a model via STDIN.

Multiple objects from multiple sources can form a scene.
The 3D viewer let's you rotate, zoom and pan the scene.
Screenshots of the visualization of the scene can be generated.

# Build

## for linux

Run `make raylib` to fetch raylib binaries and run `make` to build the project. The binary will be in the bin directory.

# Run

## on linux

Run `./bin/print3` to run the application.

# Usage

Run `./bin/print3 --help` to get all available options.

# Run examples

- STDIN: `cat examples/stdin.txt | ./bin/print3 STDIN STDIN`
- .stl (binary): `./bin/print3 examples/bunny.stl`
- .stl (ascii): `./bin/print3 examples/bottle.stl`

# TODO

The following features are planned to be added in the future:

- Support for windows
- File formats:
    - .off
    - .ply
    - .obj
- Snapping view normal to any surface
