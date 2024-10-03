# print3

Kinda like `printf` but for 3D

print3 is a visualization and debugging tool for 3D models. Multiple objects from multiple sources can be placed in one scene. A 3D viewer let's you rotate, zoom and pan the scene. It handles coloring, wireframes, scene orientation visualization and let's you create screenshots of the current view on the scene.

<p float="left">
  <img alt="3D visualization of a bunny" src="/assets/bunny.png" height="300" />
  <img alt="3D visualization of a bottle with hud" src="/assets/bottle_hud.png" height="300" />
</p>

# Supported Formats

print3 supports the following formats:
* .stl (binary and ascii)
* custom description language via the standard input

# Build

To build print3 for linux run the following commands.

```console
$ make raylib # fetches the required raylib version locally
$ make # builds the application
```
The binary will be in the `./bin` directory.

> [!NOTE]
> Currently only linux is supported.

# Usage

print3 is meant to be invoked from the command line. To print a model given in the custom description language via stdin simply run

```console
$ ./bin/print3 STDIN
```

Or you can visualize multiple models saved in one of the supported formats in one scene by running

```console
$ ./bin/print3 model1.stl model2.stl
```

To get a further usage description run the help command

``` console
$ ./bin/print3 --help
```

# Run examples

Some example models are provided in the `./examples` directory and can be visualized with print3

```console
$ cat examples/stdin.txt | ./bin/print3 STDIN STDIN # print the 2 objects within the file
$ ./bin/print3 examples/bunny.stl # print a binary stl model
$ ./bin/print3 examples/bottle.stl # print an ascii stl model
$ ./bin/print3 examples/box.off # print an off model
$ ./bin/print3 examples/cow.obj # print an obj model
```

> [!CAUTION]
> # Gotchas when using STDIN
>
> When creating a screenshot of the current view on the scene, print3 will ask for a filename on the standard input. While is is possible to pipe the output of an other program to print3's input (e.q. in order to visualize a model without persisting to disk), be aware that the screenshot feature might not behave as expected since you can not provide the input in the command line anymore.

> [!NOTE]
> Following features are planned to be added in the future:
> * Support for windows
> * Deserialzation for different formats:
>     * .ply
