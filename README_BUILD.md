# ClipEngine Build Guide

## Building ClipEngine with Samples

The root CMakeLists.txt now supports building samples alongside the main library.

### Option 1: Build Everything (Default)

```bash
# Configure (samples are built by default)
cd build
cmake .. -G "Visual Studio 17 2022" -A x64

# Build main library and SDK
cmake --build . --config Debug

# Build all samples (after SDK is installed)
cmake --build . --target build_all_samples --config Debug
```

### Option 2: Build Specific Samples

```bash
cd build

# Build only input_system_demo
cmake --build . --target build_input_system_demo --config Debug

# Build only sdk_demo (requires SDK installation)
cmake --build . --target build_sdk_demo --config Debug
```

### Option 3: Build Without Samples

```bash
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_SAMPLES=OFF
cmake --build . --config Debug
```

### Option 4: Use Build Script

```bash
# Simple build script that builds everything in order
build_all.bat
```

## Build Targets

- `clipengine` - Main ClipEngine library
- `clipengine_debug` - Debug window support library
- `install_sdk` - Install SDK to output/ClipEngineSDK-<CONFIG>
- `build_input_system_demo` - Build input system demo sample
- `build_sdk_demo` - Build SDK demo sample (depends on install_sdk)
- `build_all_samples` - Build all samples

## Sample Locations

After building:
- **input_system_demo**: `samples/input_system_demo/build/bin/Debug/input_system_demo.exe`
- **sdk_demo**: `samples/sdk_demo/build/Debug/sdk_demo.exe`
