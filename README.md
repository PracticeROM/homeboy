# Homeboy

Wii VC emulator support for [gz](https://github.com/glankk/gz) and [kz](https://github.com/krimtonz/kz).

This is a continuation of a project started by KrimtonZ (found at https://github.com/krimtonz/homeboy).
Big thanks to him for his initial work on the project.

## Prerequisites

You will need `powerpc-eabi-gcc` (provided by [devkitPro](https://devkitpro.org/wiki/devkitPro_pacman) or [wii-toolchain](https://github.com/krimtonz/wii-toolchain)) to build this project.

## Build

Run `make -j$(nproc)`. The files will be located inside the `bin/` folder.
