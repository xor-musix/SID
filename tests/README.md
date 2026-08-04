# SID Test Harness

A comprehensive test harness for comparing audio output across different SID engines (resid-0.16, resid-1.0, libresidfp).

## Overview

The test harness provides:

1. **Headless Audio Renderer** - Runs without a DAW, generating audio files directly
2. **Multiple Test Suites** - Oscillators, filters, envelopes, polyphony tests
3. **Audio Comparison** - SNR, correlation, distortion metrics
4. **Visualization** - Spectrograms, waveform overlays, difference plots
5. **CI/CD Integration** - GitHub Actions workflow for automated testing

## Directory Structure

```
tests/
├── CMakeLists.txt          # Test harness build configuration
├── tests_main.cpp          # Main test application
├── include/
│   └── SIDTestEngine.h     # Test engine wrapper
├── run_all_tests.py        # Python test orchestration
├── visualize_comparison.py # Visualization script
└── README.md               # This file
```

## Building

### Prerequisites
- CMake 3.24 or higher
- C++20 compiler
- JUCE framework (git submodule)
- Python 3.11+ (for test scripts)
- numpy, scipy, matplotlib (Python packages)

### Building the Test Library

```bash
# Configure for resid-0.16 (default)
cmake -B cmake-build-debug \
    -DCMAKE_BUILD_TYPE=Release \
    -DSID_EMULATOR=0

# Build the test library (static library)
cmake --build cmake-build-debug --target SID_tests --config Release
```

The `SID_tests` target builds a static library that contains all test code.
This library can be linked into any executable project.

### Creating a Standalone Test Executable

To create a standalone test executable without a DAW, create a new project
that links against `SID_tests`:

```cmake
# In your standalone test project CMakeLists.txt
add_executable(sid_test_runner main.cpp)

# Link against the test library
target_link_libraries(sid_test_runner PRIVATE
    ${PATH_TO_SID}/cmake-build-debug/SID_tests_artefacts/Release/libSID_tests_SharedCode.a
    # Add your JUCE modules here
)

# Include paths
target_include_directories(sid_test_runner PRIVATE
    ${PATH_TO_SID}/tests/include
    ${PATH_TO_SID}/plugin/Source
)
```

### Build for Different Engines

```bash
# resid-1.0
cmake -B cmake-build-debug \
    -DCMAKE_BUILD_TYPE=Release \
    -DSID_EMULATOR=1

# libresidfp
cmake -B cmake-build-debug \
    -DCMAKE_BUILD_TYPE=Release \
    -DSID_EMULATOR=2
```

## Usage

### Running Tests

```bash
# Run all tests
./SID_tests test all

# Run specific test suite
./SID_tests test oscillators
./SID_tests test filters
./SID_tests test envelopes
./SID_tests test polyphony

# Compare two engines
./SID_tests compare <dirA> <dirB>
```

### Using Python Scripts

```bash
# Run all tests for all engines
python tests/run_all_tests.py

# Compare specific engines
python tests/run_all_tests.py --engines resid-0.16 resid-1.0

# Generate visualizations
python tests/visualize_comparison.py test-results --all
```

### Command Line Options

```
SID Test Harness - Audio Output Comparison Tool

Usage:
    SID_tests test <type>       Run tests
    SID_tests compare <dirA> <dirB>  Compare test results

Test Types:
    all         Run all tests
    oscillators Test oscillator waveforms and frequencies
    filters     Test filter responses
    envelopes   Test ADSR envelopes
    polyphony   Test multi-voice behavior
```

## Test Suites

### Oscillator Test
Tests all four waveform types:
- Triangle
- Saw
- Square
- Noise

### Filter Test
Tests different cutoff frequencies:
- 8 different cutoff values (256-2047)
- Captures filter register values
- Verifies filter response characteristics

### Envelope Test
Tests different ADSR configurations:
- Fast attack/release
- Slow decay/sustain
- Pluck-style (no attack, quick decay)
- Pad-style (slow attack, long sustain)

### Polyphony Test
Tests multi-voice behavior:
- 1, 2, 4, and 8 voice configurations
- Voice allocation and stealing
- Chord rendering

## Comparison Metrics

### Correlation
Pearson correlation coefficient between two audio signals (-1 to 1).
- 1.0 = identical waveforms
- 0 = uncorrelated
- -1 = inverted

### SNR (Signal-to-Noise Ratio)
Measures signal quality in dB. Higher is better.

### Max Difference
Maximum absolute sample difference between two signals.

### Mean Difference
Average absolute sample difference.

## Visualization Output

The visualization script generates:
- Waveform overlays (multiple engines)
- Spectrogram comparisons
- Difference plots (waveform + histogram)
- Metric comparison charts
- HTML summary report

## CI/CD Integration

The GitHub Actions workflow (`/.github/workflows/test-sid-engines.yml`):

1. Checks out repository with submodules
2. Installs dependencies (Python packages, build tools)
3. Builds and tests each SID engine
4. Compares engine outputs
5. Generates visualizations
6. Uploads results as artifacts

## Results Directory Structure

```
test-results/
├── resid-0.16/
│   ├── osc_Triangle.wav
│   ├── osc_Saw.wav
│   ├── osc_Square.wav
│   ├── osc_Noise.wav
│   ├── filter_256.wav
│   ├── ...
│   └── compare_*.json
├── resid-1.0/
│   └── (same structure)
├── libresidfp/
│   └── (same structure)
├── figures/
│   ├── difference_*.png
│   ├── spectrogram_comparison.png
│   └── metrics_chart.png
├── comparison.json
└── index.html
```

## Adding New Tests

1. Add test function in `tests_main.cpp`
2. Register in `parseCommandLine()` method
3. Update Python script if needed
4. Update visualization if needed

## Troubleshooting

### Build Errors
- Ensure all submodules are initialized: `git submodule update --init --recursive`
- Check CMake version: `cmake --version`

### Audio Not Generated
- Check write permissions on output directory
- Verify test executable path

### Visualization Errors
- Install Python dependencies: `pip install numpy scipy matplotlib`
- Check matplotlib backend: `matplotlib.use('Agg')` for headless servers

## License

Same as main SID plugin project.