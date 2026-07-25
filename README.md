# SID

VST / AU Commodore 64 SID emulation

![Build Windows](https://github.com/FigBug/SID/workflows/Build%20Windows/badge.svg "Build Windows")
![Build macOS](https://github.com/FigBug/SID/workflows/Build%20macOS/badge.svg "Build macOS")
![Build Linux](https://github.com/FigBug/RP2A03/workflows/Build%20Linux/badge.svg "Build Linux")

---

## Building on Linux

### Prerequisites

- CMake 3.24.0 or later
- GCC 9 or later (C++20 support required)
- Git
- curl development libraries (`libcurl-dev` on Debian/Ubuntu)
- pkg-config

### Installation of Dependencies (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install build-essential cmake git libcurl-dev pkg-config
```

### Building from Source

1. Clone the repository (including submodules):

```bash
git clone --recursive https://github.com/xor-musix/SID.git
cd SID
```

2. Configure with CMake using the ninja-gcc preset:

```bash
cmake --preset ninja-gcc
```

3. Build the project:

```bash
cmake --build --preset ninja-gcc --config Release
```

4. (Optional) Create a Debian package:

```bash
cd Builds/ninja-gcc
cpack -G DEB -C Release
```

The package will be created in the `Builds/ninja-gcc` directory.

### Running the Standalone Plugin

After building, the standalone executable can be found at:

```
Builds/ninja-gcc/SID_artefacts/Release/SID
```

Run it directly:

```bash
./Builds/ninja-gcc/SID_artefacts/Release/SID
```

### Plugin Locations

After building, the plugins are available in:

- **VST**: `Builds/ninja-gcc/SID_artefacts/Release/VST/libSID.so`
- **VST3**: `Builds/ninja-gcc/SID_artefacts/Release/VST3/SID.vst3`
- **LV2**: `Builds/ninja-gcc/SID_artefacts/Release/LV2/SID.lv2`
- **CLAP**: `Builds/ninja-gcc/SID_artefacts/Release/CLAP/SID.clap`

### Installation Paths

When installing the Debian package or manually:

- **VST**: `/usr/lib/vst/SID.so`
- **VST3**: `/usr/lib/vst3/SID.vst3`
- **LV2**: `/usr/lib/lv2/SID.lv2`
- **CLAP**: `/usr/lib/clap/SID.clap`
- **Presets**: `/usr/share/SocaLabs/SID/Presets/`

## Building on Other Platforms

### Windows

Use Visual Studio 2022 or later:

```bash
cmake --preset vs
cmake --build --preset vs --config Release
```

### macOS

Use Xcode:

```bash
cmake --preset xcode
cmake --build --preset xcode --config Release
```

## License

This project is licensed under the terms found in the LICENSE file.

## Author

SocaLabs - https://socalabs.com/