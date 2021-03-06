# ESSA build instructions

This guide will show you how to build and run ESSA from the source.

## Install required third-party dependencies

* `cmake` - as a build system
* Some "lower-level" build system, we use `ninja` but `make` or Visual Studio should work
* `SFML` - for GUI
* `GLEW` - currently for nothing but is linked
* `PythonLibs` - for PySSA

Arch Linux/Manjaro:

```sh
sudo pacman -Ss base-devel sfml glew python3
```

## Build and run EssaGUI

EssaGUI is a GUI framework written specifically for Essa. You can download it from the [GitHub repository](https://github.com/essa-software/EssaGUI).

TODO: make it a submodule

From the project root directory:
```sh
git clone git@github.com:essa-software/EssaGUI.git
cd EssaGUI
mkdir build
cd build
cmake .. -GNinja
ninja
sudo ninja install
cd ../../
```

## Setup CMake

From the project root directory:
```sh
mkdir build
cd build
cmake .. -GNinja
```

(Replace `-GNinja` with your generator)

## Build the project

From the `build` directory:
```sh
ninja
```

## Run

From the `build` directory (This is required because otherwise the app won't see resources, see #10):
```sh
./out
```
