<h1 align="center">
  R-Type - EPITECH PROJECT 2025<br>
  <img src="https://raw.githubusercontent.com/catppuccin/catppuccin/main/assets/palette/macchiato.png" width="600px"/>
  <br>
</h1>

<div align="center">
  <p></p>
  <div align="center">
     <a href="https://github.com/Leorevoir/R-Type/stargazers">
        <img src="https://img.shields.io/github/stars/Leorevoir/R-Type?color=F5BDE6&labelColor=303446&style=for-the-badge&logo=starship&logoColor=F5BDE6">
     </a>
     <a href="https://github.com/Leorevoir/R-Type/">
        <img src="https://img.shields.io/github/repo-size/Leorevoir/R-Type?color=C6A0F6&labelColor=303446&style=for-the-badge&logo=github&logoColor=C6A0F6">
     </a>
     <a href="https://github.com/Leorevoir/R-Type/blob/main/LICENSE">
        <img src="https://img.shields.io/static/v1.svg?style=for-the-badge&label=License&message=GPL3&colorA=313244&colorB=F5A97F&logo=unlicense&logoColor=F5A97F&"/>
     </a>
  </div>
  <br>
</div>

<p align="center">
  🎮 R-Type 🎮<br>
  A cross-platform video game made with
    <a href="https://github.com/Leorevoir/R-Engine">
      R-Engine
    </a>
</p>

## Description

> TODO

## Installation

Clone the repo with its submodules.

```bash
git clone --recurse-submodules -j$(nproc) git@github.com:Leorevoir/R-Type.git
```

## Install Git LFS

This project stores large binary assets with Git LFS. Please install Git LFS and fetch LFS objects after cloning:

```sh
# install & enable LFS (run once per machine)
git lfs install

cd R-Type/
git lfs fetch --all
git lfs checkout
```

### Build

Build the project using our multithreading build `bash` script:

```bash
./build.sh
```

Or manually run `cmake`:

```bash
mkdir build/ && cd build/
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
cd ..
```

### Run and play!

After building the project, you can run the game by executing the binary:

```bash
./r-type
```

#### Controls

The game supports both keyboard and gamepad controls.

| Action               | Keyboard              | Gamepad (Xbox/PlayStation) |
| -------------------- | --------------------- | -------------------------- |
| Move                 | `W`, `A`, `S`, `D`    | Left Stick                 |
| Fire / Charge Cannon | `Spacebar` (Tap/Hold) | `A` / `X` (Tap/Hold)       |
| Launch/Recall Force  | `Left Shift`          | `B` / `Circle`             |
| Pause                | `Escape`              | `Start`                    |

##### Debug Controls

If the `DebugPlugin` is enabled in `src/Main.cpp`, you can use the following keys to switch levels during gameplay:

- `F1`: Switch to Level 1
- `F2`: Switch to Level 2
- `F3`: Switch to Level 3

## Graphical backend

- [**R-Engine**](https://github.com/Leorevoir/R-Engine)

## Development

THANKS for developping with us !

> TODO: add CODE_OF_CONDUCT.md & CONTRIBUTING.md

## Tested on

- MacOS
- Windows
- Debian
- EndeavourOS
- Arch
- Ubuntu
- Fedora

## License

> TODO (GNU-GPL3.0)
