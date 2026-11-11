# MicroStitch v3 - Pure Native C++ Standalone Desktop Engine

## Overview
This directory preserves the 100% native C++17 standalone desktop application for real-time USB microscope image stitching. It completely eliminates all Python, GIL, and DLL dependencies by compiling the Fourier phase correlation and dynamic multi-band blending algorithms directly into a high-performance C++ executable.

## Contents
- `CMakeLists.txt`: Standalone CMake 3.16+ build configuration targeting MSVC and OpenCV 3.4.11.
- `include/microstitch/stitcher.h`: Public C++ class interface (`microstitch::Stitcher`).
- `src/`:
  - `main.cpp`: Standalone runner managing dual OpenCV highgui windows (Live Microscope Camera feed + Live Stitching Mosaic).
  - `registration.h` / `registration.cpp`: 2D subpixel Fourier phase correlation (`cv::phaseCorrelate`) and SIFT fallback with Hann window filtering.
  - `mosaic.h` / `mosaic.cpp`: Dynamic bounding box expansion and global coordinate translation.
  - `mosaic_renderer.h` / `mosaic_renderer.cpp`: Continuous linear distance transform feathering for seamless mosaic synthesis.
  - `stitcher.h` / `stitcher.cpp`: High-level thread-safe C++ controller.
- `live_mosaic_458.png`: Real stitched high-resolution output sample demonstrating multi-frame reconstruction from 458 video frames.

## Build Instructions
```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
    -S . -B build -G "Visual Studio 18 2026" -A x64
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
    --build build --config Release
```

## Status
- **Status**: Fully verified and independently buildable standalone C++ executable.
