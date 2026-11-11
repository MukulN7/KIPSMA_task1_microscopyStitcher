# MicroStitch C++ DLL Integration & Handoff Guide

## Project Purpose
MicroStitch is a 2D microscope image stitching pipeline designed for real-time mosaic construction from continuous camera frames. This package provides **`Stitcher.dll`**, a pure C-compatible Win32 dynamic link library that allows an MFC (Microsoft Foundation Classes) C++ desktop application to consume the Python MicroStitch pipeline without exposing CPython, NumPy, OpenCV, or SIFT algorithm implementation details across the DLL boundary.

---

## Architecture & Data Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    MFC Application (C++)                    │
│      - Captures camera frames (cv::Mat / BGR bytes)         │
│      - Manages UI windows and user controls (Reset/Save)   │
└──────────────────────────────┬──────────────────────────────┘
                               │ C API (stitcher_api.h)
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                        Stitcher.dll                         │
│  - Exported C API functions                                 │
│  - Internal PyGILState thread safety wrappers               │
│  - Embedded CPython interpreter runtime                     │
└──────────────────────────────┬──────────────────────────────┘
                               │ Python C API
                               ▼
┌─────────────────────────────────────────────────────────────┐
│              MicroStitch Engine (microstitch)               │
│  - MicroStitchEngine stateful wrapper                       │
│  - RegistrationEngine (SIFT + RANSAC + Phase Correlation)   │
│  - MosaicManager (Global coordinate accumulation)           │
│  - MosaicCanvasRenderer (2D linear distance feathering)     │
└─────────────────────────────────────────────────────────────┘
```

---

## The DLL C API Interface

The public interface is defined in `native/include/stitcher_api.h`.

```cpp
#pragma once

#ifdef _WIN32
  #ifdef STITCHER_EXPORTS
    #define STITCHER_API __declspec(dllexport)
  #else
    #define STITCHER_API __declspec(dllimport)
  #endif
#else
  #define STITCHER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

STITCHER_API void* Stitcher_Create(int width, int height, int channels);

STITCHER_API int Stitcher_ProcessFrame(
    void* handle,
    const unsigned char* data,
    int width,
    int height,
    int channels,
    int stride);

STITCHER_API int Stitcher_GetMosaic(
    void* handle,
    const unsigned char** data,
    int* width,
    int* height,
    int* channels,
    int* stride);

STITCHER_API void Stitcher_Reset(void* handle);
STITCHER_API void Stitcher_Destroy(void* handle);

#ifdef __cplusplus
}
#endif
```

### Function Specifications

| Function | Return | Description |
|----------|--------|-------------|
| **`Stitcher_Create`** | `void*` | Allocates and returns an opaque handle to a persistent `MicroStitchEngine` instance. Returns `NULL` on failure. |
| **`Stitcher_ProcessFrame`** | `int` | Passes one raw frame buffer into the stitching engine.<br>• `1` = Frame accepted & added to mosaic.<br>• `0` = Frame rejected (quality gate / stationary).<br>• `-1` = Error (invalid arguments or runtime error). |
| **`Stitcher_GetMosaic`** | `int` | Retrieves the current rendered mosaic buffer and dimensions.<br>• `1` = Valid mosaic available (`*data` populated).<br>• `0` = No mosaic ready (`*data = NULL`).<br>• `-1` = Error. |
| **`Stitcher_Reset`** | `void` | Clears all accumulated mosaic tiles and resets position to origin $(0, 0)$. |
| **`Stitcher_Destroy`** | `void` | Releases Python references and frees internal engine resources. Handle becomes invalid. |

---

## Frame Data Format

All image frame buffers passed into `Stitcher_ProcessFrame` must follow this convention:
- **Channel Order**: BGR (Standard OpenCV format)
- **Data Type**: `uint8` (`unsigned char`)
- **Dimensions**: `width` (pixels), `height` (pixels), `channels` (typically `3`)
- **Stride**: `stride` = Bytes per row (`width * channels` for tight packing, or `frame.step` for strided DIBs)

---

## Memory Ownership Policy

1. **Input Frame Memory (`ProcessFrame`)**:
   - The DLL creates an independent, owned copy of the frame array in Python memory before processing.
   - The MFC application retains **100% ownership** of its camera frame buffer (`cv::Mat`) and may reuse or free it immediately after `Stitcher_ProcessFrame()` returns.

2. **Mosaic Buffer Memory (`GetMosaic`)**:
   - The returned byte pointer (`const unsigned char** data`) points directly to the DLL's cached NumPy array buffer.
   - The buffer is **owned by the DLL** and remains valid until the next call to `Stitcher_ProcessFrame`, `Stitcher_Reset`, or `Stitcher_Destroy`.
   - **Important**: The MFC application **must not free** this pointer. If the MFC application needs to store the mosaic canvas across process cycles, it **must make a copy** (e.g., `cv::Mat mosaic = cv::Mat(mh, mw, CV_8UC3, (void*)data, ms).clone()`).

---

## Minimal C++ Integration Code Example

```cpp
#include <iostream>
#include <opencv2/opencv.hpp>
#include "stitcher_api.h"

int main() {
    // 1. Create stitcher instance for 640x360 3-channel input
    void* handle = Stitcher_Create(640, 360, 3);
    if (!handle) {
        std::cerr << "Failed to initialize Stitcher DLL!" << std::endl;
        return 1;
    }

    cv::VideoCapture cap(1); // Open USB microscope
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 360);

    cv::Mat frame;
    while (cap.read(frame)) {
        // 2. Feed frame into DLL
        int rc = Stitcher_ProcessFrame(
            handle,
            frame.data,
            frame.cols,
            frame.rows,
            frame.channels(),
            static_cast<int>(frame.step)
        );

        // 3. Query updated mosaic
        const unsigned char* mosaic_bytes = nullptr;
        int mw = 0, mh = 0, mc = 0, ms = 0;
        int has_mosaic = Stitcher_GetMosaic(handle, &mosaic_bytes, &mw, &mh, &mc, &ms);

        if (has_mosaic && mosaic_bytes != nullptr) {
            // 4. Wrap returned buffer in cv::Mat (or clone if persistent copy needed)
            cv::Mat mosaic(mh, mw, CV_8UC3, const_cast<unsigned char*>(mosaic_bytes), ms);
            cv::imshow("Mosaic", mosaic);
        }

        cv::imshow("Live Camera", frame);

        int key = cv::waitKey(1);
        if (key == 27 || key == 'q') break;
        if (key == 'r') {
            // 5. Mid-session reset
            Stitcher_Reset(handle);
        }
    }

    // 6. Destroy handle on exit
    Stitcher_Destroy(handle);
    cv::destroyAllWindows();
    return 0;
}
```

---

## Runtime Requirements & Deployment Environment

- **Operating System**: Windows (64-bit AMD64)
- **Python Environment**: CPython 3.14 (or matching 64-bit Python installation containing `python314.dll`)
- **Python Packages**: `numpy`, `opencv-python`, `tifffile` (see `requirements.txt`)
- **Environment Variable**: `STITCHER_PROJECT_ROOT`
  - Set `STITCHER_PROJECT_ROOT` to the absolute path of the directory containing the `microstitch/` package folder prior to calling `Stitcher_Create()`.
  - Alternatively, ensure `microstitch` is installed in Python's site-packages or present in the current working directory.

---

## Verification Performed

1. **Python Engine Verification**: Stateful `MicroStitchEngine` tested with real USB microscope camera hardware at 640x360 resolution (`scratch_test_engine_camera.py`).
2. **Native C++ DLL Verification**: `Stitcher.dll` compiled and verified with standalone C++ test app (`test_dll_cpp.exe`).
3. **Python `ctypes` Smoke Test**: End-to-end dynamic loading and execution tested via CPython `ctypes`.
4. **Physical Microscope DLL Test**: Live physical microscope test executed with `test_microscope_dll.exe`.
5. **Frame Buffer Ownership Fix**: Identified and resolved zero-copy buffer mutation bug by creating an explicit owned NumPy copy inside `Stitcher_ProcessFrame()`, ensuring historical tiles in `MosaicManager` remain untouched when C++ reuses its camera frame buffer.

---

## Next Steps for the MFC Application Developer

1. **Link Dependencies**:
   - Add `native/include` to C++ Include Directories.
   - Link `native/bin/Stitcher.lib` in your Visual Studio / MFC project properties.
   - Ensure `Stitcher.dll` and `python314.dll` are present in the executable output directory or system `PATH`.
2. **Set Environment Variable**:
   - Call `SetEnvironmentVariable("STITCHER_PROJECT_ROOT", "C:\\Path\\To\\Handoff")` at application startup.
3. **Integrate Frame Loop**:
   - Instantiate stitcher via `Stitcher_Create(width, height, channels)`.
   - Pass camera frame buffers (`cv::Mat`) to `Stitcher_ProcessFrame()`.
   - Retrieve mosaic via `Stitcher_GetMosaic()` and display in MFC `CWnd` / Picture Control.
   - Wire UI buttons to `Stitcher_Reset()` and `Stitcher_Destroy()`.
