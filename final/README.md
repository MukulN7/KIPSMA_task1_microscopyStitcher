# KStitcher - Production MFC Windows Desktop Application

## Overview
This directory contains the final production deliverable: **KStitcher**, a native Microsoft Foundation Classes (MFC) C++ desktop GUI application for industrial and laboratory microscope image stitching.

It embeds the native C++ Fourier phase-correlation stitching engine into a full Windows graphical user interface with real-time video preview, interactive canvas panning/zoom, live controls (Start/Stop, Reset, Save Mosaic, Adjust Overlap Sensitivity), and status telemetry.

## Directory Contents
- `KStitcher.sln`: Visual Studio solution file configured for MSVC x64.
- `KStitcher/`:
  - `KStitcher.vcxproj`, `KStitcher.vcxproj.filters`: Project configurations.
  - `KStitcher.h` / `KStitcher.cpp`: MFC WinMain application class (`CKStitcherApp`).
  - `KStitcherDlg.h` / `KStitcherDlg.cpp`: Main dialog implementation with GUI event handlers, timer-driven camera acquisition, and OpenCV rendering on MFC picture controls.
  - `CvvImage.h` / `CvvImage.cpp`: High-performance GDI/OpenCV bridge rendering `cv::Mat` to Windows Device Contexts (`HDC`).
  - `stitcher.h` / `stitcher.cpp`: Core C++ stitching controller.
  - `registration.h` / `registration.cpp`: Subpixel translation estimation engine.
  - `mosaic.h` / `mosaic.cpp`: Dynamic mosaic coordinate mapping and canvas bounds tracking.
  - `mosaic_renderer.h` / `mosaic_renderer.cpp`: Dynamic weighted-blend mosaic renderer.
  - `MyStitcher.h`: Adapter interface for dialog-to-stitcher binding.
  - `res/`, `KStitcher.rc`, `resource.h`: Windows icons, dialog templates, and resources.

## Building in Visual Studio
1. Install **Visual Studio 2019 / 2022 / 2026** with the *Desktop development with C++* workload and the *MFC for latest build tools* component.
2. Install **OpenCV 3.4.11 (x64)** under `C:\tools\opencv3411` (or update include/lib directories in Project Properties).
3. Open `KStitcher.sln` in Visual Studio.
4. Select configuration `Release` and platform `x64`.
5. Build Solution (`Ctrl+Shift+B`).
