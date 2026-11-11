# MicroscopeStitcher - Live C++ Microscope Stitching Standalone Demo

A 100% native C++ standalone desktop application for real-time USB microscope image stitching using 2D phase correlation registration and dynamic multi-band blending.

---

## Architecture & Features

- **Language**: Native C++ (C++17)
- **OpenCV Version**: OpenCV 3.4.11 (x64)
- **Dependencies**: Native OpenCV 3.4.11 runtime DLLs only.
- **Independence**:
  - **No DLL required**: The core stitching engine (`registration.cpp`, `mosaic.cpp`, `mosaic_renderer.cpp`, `stitcher.cpp`) is compiled **directly into `MicroscopeStitcher.exe`**.
  - **No Python / No Python Embedding**.
  - **No MFC dependency**.

---

## Project Directory Structure

```text
MicroscopeStitcher_Standalone/
├── CMakeLists.txt                 (Build Script for Standalone Executable)
├── README.md                      (Build & Run Documentation)
├── include/
│   └── microstitch/
│       └── stitcher.h             (Public C++ API Header)
└── src/
    ├── main.cpp                   (Standalone Live Camera & Dual-Window UI Runner)
    ├── registration.h / .cpp     (Phase Correlation & Alignment Engine)
    ├── mosaic.h / .cpp           (Global Tile Positioning & Canvas State)
    ├── mosaic_renderer.h / .cpp  (Seam-Free Multi-Band / Linear Blending)
    └── stitcher.h / .cpp         (High-Level C++ Stitcher Interface)
```

---

## Prerequisites & Setup

1. **Visual Studio 2026 / MSVC** (x64 compiler)
2. **CMake 3.16+**
3. **OpenCV 3.4.11 x64 Installation**:
   Ensure OpenCV 3.4.11 is installed at `C:\tools\opencv3411` or pass `-DOpenCV_DIR=<path>` during CMake configuration.

---

## Build Instructions

### 1. Configure CMake
Open PowerShell or Command Prompt:
```cmd
cd MicroscopeStitcher_Standalone
cmake -B build -G "Visual Studio 18 2026" -A x64 -DOpenCV_DIR=C:\tools\opencv3411
```

### 2. Compile Release Executable
```cmd
cmake --build build --config Release
```

The output executable will be generated at:
`build\bin\Release\MicroscopeStitcher.exe`

---

## Deployment & OpenCV Runtime DLLs

Copy the OpenCV 3.4.11 runtime DLLs from `C:\tools\opencv3411\bin\` to the directory containing `MicroscopeStitcher.exe`:
- `opencv_core3411.dll`
- `opencv_imgproc3411.dll`
- `opencv_features2d3411.dll`
- `opencv_calib3d3411.dll`
- `opencv_imgcodecs3411.dll`
- `opencv_highgui3411.dll`
- `opencv_videoio3411.dll`

---

## Running the Application

### Default Camera (Camera 0)
```cmd
build\bin\Release\MicroscopeStitcher.exe
```

### Custom Camera Index (e.g. Camera 1)
```cmd
build\bin\Release\MicroscopeStitcher.exe 1
```

---

## Expected Behavior & Dual Windows

Upon launching, the program creates **two real-time OpenCV windows**:

1. **Current Frame**: Displays the live USB microscope camera stream.
2. **Live Mosaic**: Displays the continuously growing stitched panoramic mosaic as the slide is moved across the microscope stage.

---

## Keyboard Controls

| Key | Action |
| :--- | :--- |
| **ESC** / **Q** | Quit the application cleanly |
| **R** | Reset the current mosaic and clear stored tiles |
| **S** | Save the current stitched mosaic image to disk (`live_mosaic_<frame>.png`) |

