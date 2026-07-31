# SID

VST / AU / LV2 / CLAP Commodore 64 SID Emulation

![Build Windows](https://github.com/FigBug/SID/workflows/Build%20Windows/badge.svg "Build Windows")
![Build macOS](https://github.com/FigBug/SID/workflows/Build%20macOS/badge.svg "Build macOS")
![Build Linux](https://github.com/FigBug/RP2A03/workflows/Build%20Linux/badge.svg "Build Linux")

## Selecting a SID Engine

This plugin supports three different SID chip emulators, each with distinct characteristics:

| Feature | resid-0.16 | resid-1.0 | libresidfp |
|---------|-----------|-----------|------------|
| **Origin** | Dag Lem (2004) | VICE project | Fork of 0.16 by Antti Lankila/Leandro Nini |
| **Namespace** | Global | `reSID` | `reSIDfp` |
| **Arithmetic** | Fixpoint + float | Fixpoint + float | Floating-point |
| **Build Type** | Direct sources | VICE submodule | Separate library |
| **Filter Model** | Two-integrator-loop biquadratic | Improved pipeline | Parametrized, real chip data |
| **Waveforms** | Calculated | Calculated + patches | Parametrized from samples |
| **Resampling** | Single-step | Improved | Two-step (lower order) |
| **DAC Emulation** | Basic | Basic | Non-linearity, distortion |
| **C++ Standard** | C++98 compatible | C++11+ | C++17 required |

### Description of Each Engine

#### **resid-0.16** (Default, `SID_EMULATOR=0`)
- Original reSID library by Dag Lem - the canonical implementation
- Uses **fixpoint arithmetic** (16.16 bit) for timing calculations
- **Spline interpolation** for smooth waveform generation
- Filter modeled as two-integrator-loop biquadratic circuit
- Resampling modes: SAMPLE_FAST, SAMPLE_INTERPOLATE, SAMPLE_RESAMPLE, SAMPLE_RESAMPLE_FASTMEM
- Located at: `plugin/Source/3rdparty/resid-0.16/`

#### **resid-1.0** (`SID_EMULATOR=1`)
- Embedded VICE submodule - the most actively maintained version
- **Pipeline modeling** for improved timing accuracy
- Additional features:
  - `set_voice_mask()` - voice masking control
  - `adjust_filter_bias()` - filter bias adjustment
  - `enable_raw_debug_output()` - debugging capabilities
- Enhanced State class with: `write_pipeline`, `voice_mask`, `shift_pipeline`, etc.
- Located at: `plugin/Source/3rdparty/resid-1.0/vice-git/`

#### **libresidfp** (`SID_EMULATOR=2`)
- Fork of reSID 0.16 focused on floating-point operations and filter accuracy
- **Parametrized waveform model** based on real chip samplings
- **Enhanced filter modeling**:
  - Filter distortion
  - 6581 DAC non-linearity
  - 6581 DAC distortion
  - 6581 DC drift
  - Oscillator leak
  - Configurable combined waveform strength
- **Two-step resampling** using lower-order filters
- **Fritsch-Carlson interpolation** for opamp values (preserves monotonicity)
- Located at: `plugin/Source/3rdparty/resid-fp/libresidfp/`

### Switching Between Engines

The SID engine is selected at build time via the `SID_EMULATOR` CMake option:

```bash
# resid-0.16 (default)
cmake --preset ninja-gcc

# resid-1.0
cmake --preset ninja-gcc -DSID_EMULATOR=1

# libresidfp
cmake --preset ninja-gcc -DSID_EMULATOR=2
```

All engines are accessed through compatibility headers that expose identical API identifiers to the plugin code.

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

## Disclaimer

This repo (fork) is developed with help from AI tools.