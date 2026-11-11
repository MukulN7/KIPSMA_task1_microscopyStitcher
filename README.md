# Microscope Image Stitching (MicroStitch / KStitcher)

A real-time computer vision system that captures continuous video frames from a fixed USB microscope camera as a specimen slide is moved and combines consecutive overlapping frames into an expanded, high-resolution mosaic.

Developed for **Kips Microanalysis**.

---

## 1. Overview
In optical microscopy, the field of view (FOV) is constrained by optical magnification: higher magnification reveals microstructural details but drastically reduces visible specimen area. **MicroStitch / KStitcher** solves this fundamental constraint by continuously registering incoming video frames from a USB microscope camera as an operator traverses a metallurgical or biological specimen slide, reconstructing a wide-area high-resolution mosaic in real time.

The project traces the complete engineering progression from initial algorithmic research in Python to a hybrid C-API DLL wrapper, a standalone native C++ executable, and finally a fully-featured Microsoft Foundation Classes (MFC) Windows desktop application.

---

## 2. Problem Statement
Manual microscopic examination requires navigating across large slide areas while attempting to maintain spatial orientation and document macroscopic defects. Capturing multiple individual snapshots requires tedious manual post-processing and alignment. The system must:
- Detect translation between consecutive microscope frames at 30+ FPS without dropped frames.
- Reject camera motion blur, vibration, and non-overlapping frames.
- Formulate a seamless global coordinate canvas that dynamically expands as new regions are explored.
- Blend overlapping regions without visible boundary seams or ghosting artifacts.
- Operate directly in industrial laboratory workstations running Windows.

---

## 3. Objectives
- **Subpixel Registration Accuracy:** Determine frame-to-frame shifts ($\Delta x, \Delta y$) with subpixel precision using frequency-domain Phase Correlation.
- **Robustness to Illumination & Texture Variance:** Suppress sensor noise and edge vignetting using apodization windows (Hann windowing).
- **Dynamic Seam Feathering:** Apply 2D Euclidean distance transform weighting to achieve artifact-free alpha blending across overlapping frames.
- **Production Desktop Integration:** Deliver both an automated standalone test harness and an interactive MFC graphical desktop application.

---

## 4. System Architecture

```text
[USB Microscope Camera / Video Source]
                 │
                 ▼
     [Frame Preprocessing]
     ├─ Grayscale Conversion
     ├─ Optical Jitter / Motion Filtering
     └─ Hann Window Apodization
                 │
                 ▼
     [Translation Estimation]
     ├─ 2D FFT & Cross-Power Spectrum
     ├─ Phase Correlation Peak Detection
     └─ Subpixel Parabolic / Centroid Refinement (SIFT Fallback)
                 │
                 ▼
     [Mosaic Canvas Manager]
     ├─ Accumulated Global Coordinates
     ├─ Dynamic Bounding Box Expansion
     └─ Memory-Efficient Canvas Allocation
                 │
                 ▼
     [Seam-Free Blender]
     ├─ Euclidean Distance Transform
     └─ Dynamic Linear Distance Feathering
                 │
                 ▼
[MFC Desktop GUI / OpenCV Interactive Display / File Export]
```

---

## 5. Technical Approach & Mathematical Model
Because the microscope camera is mounted perpendicularly on a fixed vertical axis above the translation stage:
1. **Motion Model**: The physical system is modeled as pure 2D rigid translation:
   $$\mathbf{x}' = \mathbf{x} + \mathbf{t}, \quad \mathbf{t} = \begin{bmatrix} \Delta x \\ \Delta y \end{bmatrix}$$
2. **Frequency-Domain Phase Correlation**:
   Given two consecutive frames $f_1(x, y)$ and $f_2(x, y) = f_1(x - \Delta x, y - \Delta y)$, their 2D Discrete Fourier Transforms $F_1(u, v)$ and $F_2(u, v)$ satisfy the Fourier Shift Theorem:
   $$F_2(u, v) = F_1(u, v) e^{-i 2\pi (u \Delta x / M + v \Delta y / N)}$$
   The normalized cross-power spectrum is computed as:
   $$R(u, v) = \frac{F_1(u, v) \cdot F_2^*(u, v)}{|F_1(u, v) \cdot F_2^*(u, v)|}$$
   The inverse Fourier transform $\mathcal{F}^{-1}\{R(u, v)\}$ yields a Dirac delta function centered at the translation vector $(\Delta x, \Delta y)$.
3. **Apodization (Hann Windowing)**:
   To eliminate spectral leakage caused by rectangular image boundaries, frames are weighted prior to FFT:
   $$w(x, y) = \frac{1}{4} \left[1 - \cos\left(\frac{2\pi x}{M-1}\right)\right] \left[1 - \cos\left(\frac{2\pi y}{N-1}\right)\right]$$
4. **Feathered Multi-Band Blending**:
   Overlapping regions are blended using weights derived from distance-to-edge transforms:
   $$I_{\text{mosaic}}(x, y) = \frac{\sum_k w_k(x, y) I_k(x, y)}{\sum_k w_k(x, y)}$$

---

## 6. Algorithms and Methods
- **Motion Gating:** Compares absolute frame differences to threshold micro-vibrations before triggering FFT calculations.
- **Phase Correlation Peak Analysis:** Evaluates peak sharpness and signal-to-noise ratio to reject non-overlapping or out-of-focus frames.
- **SIFT / RANSAC Feature Matching Fallback:** When low-frequency background patterns cause phase ambiguity, sparse feature matching confirms rigid alignment.
- **Dynamic Linear Feathering:** Eliminates exposure-step seams caused by non-uniform microscope lighting.

---

## 7. Technologies and Dependencies
- **Languages:** C++ (C++17), Python (v1 prototype)
- **Computer Vision:** OpenCV 3.4.11 (`core`, `imgproc`, `imgcodecs`, `highgui`, `features2d`, `videoio`, `calib3d`)
- **Build System:** CMake 3.16+ (Visual Studio 2019/2022/2026 generator)
- **Desktop Framework:** Microsoft Foundation Classes (MFC) Win32/x64
- **Operating System:** Windows 10 / Windows 11 (64-bit)

---

## 8. Repository Structure

```text
KIPSMA_task1_microscopyStitcher/
├── README.md                      # Comprehensive project documentation
├── .gitignore                     # Git ignore rules for MSVC, CMake, Python
├── docs/                          # Integration manuals and technical notes
│   ├── MFC_Integration_Steps.pdf
│   ├── MicroscopeStitcher_Code_Walkthrough.docx
│   ├── microstichintegration07.pdf
│   ├── microstitchintegration08.pdf
│   └── MicroStitch_MFC_Integration.pdf
├── final/                         # Production MFC Desktop GUI Application
│   ├── README.md                  # MFC build and setup guide
│   ├── KStitcher.sln              # Visual Studio solution
│   └── KStitcher/                 # MFC source files, dialogs, resources
│       ├── KStitcherDlg.cpp/h     # UI controls, camera loop, display
│       ├── CvvImage.cpp/h         # OpenCV to Windows GDI renderer
│       ├── stitcher.cpp/h         # Stitching engine coordinator
│       ├── registration.cpp/h     # Phase correlation engine
│       ├── mosaic.cpp/h           # Canvas management
│       └── mosaic_renderer.cpp/h  # Seam-free dynamic blender
└── versions/                      # Meaningful evolutionary implementations
    ├── v1_python_prototype/       # Python prototype with Streamlit & tests
    │   ├── README.md
    │   ├── microstitch/           # Python algorithms package
    │   ├── tests/                 # Unit test suite
    │   └── PROJECT_CONTEXT.md     # Initial specification document
    ├── v2_mfc_dll_integration/    # C-compatible DLL wrapping embedded Python
    │   ├── README.md
    │   ├── MFC_STITCHER_HANDOFF.md
    │   ├── native/                # stitcher_api.h, Stitcher.dll, Stitcher.lib
    │   └── microstitch/           # Python engine backend
    └── v3_standalone_cpp/         # Native C++ standalone executable
        ├── README.md
        ├── CMakeLists.txt         # Standalone CMake build
        ├── include/microstitch/   # C++ stitcher class interface
        ├── src/                   # C++ engine sources and dual-window runner
        └── live_mosaic_458.png    # Verified 458-frame reconstructed sample
```

---

## 9. Version History & Evolution

| Stage | Identifier | Implementation | Purpose & Milestone |
| :--- | :--- | :--- | :--- |
| **v1** | `versions/v1_python_prototype` | Python 3.10 + OpenCV | Algorithmic exploration, proof-of-concept phase correlation, and Streamlit demo. |
| **v2** | `versions/v2_mfc_dll_integration` | C++ DLL + Embedded Python | Created pure C API bridge (`stitcher_api.h`) for MFC consumption. |
| **v3** | `versions/v3_standalone_cpp` | Native C++17 (CMake) | 100% C++ implementation without Python/DLL dependencies; dual-window live preview. |
| **Final** | `final/` (KStitcher) | Native C++ MFC Solution | Production Windows GUI application with interactive camera controls, live mosaic display, and direct export. |

---

## 10. Installation & Build Instructions

### Building the Standalone C++ Runner (v3)
```powershell
# Open PowerShell in versions/v3_standalone_cpp
cd versions/v3_standalone_cpp

# Configure CMake with Visual Studio MSVC x64
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
    -S . -B build -G "Visual Studio 18 2026" -A x64

# Compile Release binary
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
    --build build --config Release
```

### Building the Final MFC Application (`final/`)
1. Ensure **Visual Studio** is installed with the *Desktop development with C++* workload and *MFC for latest build tools*.
2. Ensure **OpenCV 3.4.11** is installed at `C:\tools\opencv3411`.
3. Open `final/KStitcher.sln`.
4. Set configuration to `Release` | `x64`.
5. Build solution (`Build -> Build Solution` or `Ctrl+Shift+B`).

---

## 11. Execution Instructions
- **Standalone Engine:** Run `versions/v3_standalone_cpp/build/bin/Release/MicroscopeStitcher.exe`. Two windows appear: *Microscope Camera* (live feed) and *Live Mosaic* (accumulated stitched panorama). Press `ESC` to quit and export `live_mosaic_output.png`.
- **MFC Desktop App:** Run `final/KStitcher/Release/KStitcher.exe`. Use the graphical buttons to start camera streaming, initialize mosaic stitching, pause, reset canvas, or export high-resolution bitmap/JPEG images.

---

## 12. Testing Instructions
- **Python Prototype Unit Tests:**
  ```powershell
  cd versions/v1_python_prototype
  pytest tests/
  ```
- **C++ Standalone Execution Test:**
  Launch `MicroscopeStitcher.exe` with a connected USB camera or simulated video feed to verify phase correlation and dynamic canvas growth.

---

## 13. Input & Output Expectations
- **Input:** Live USB microscope video stream (640x480, 1280x720, or 1920x1080 24-bit BGR).
- **Output:** Stitched multi-megapixel composite image (PNG / TIFF / JPEG) with feathering across all tile boundaries. See `versions/v3_standalone_cpp/live_mosaic_458.png` for a real 458-frame verification output.

---

## 14. Limitations & Future Improvements
- **Current Limitations:**
  - Designed exclusively for planar 2D rigid translations ($\Delta x, \Delta y$); does not estimate rotation or out-of-plane tilting.
  - Requires continuous overlap (minimum 20% overlap between adjacent frames).
- **Future Improvements:**
  - Integration with stage motorized encoders for sub-micrometer absolute coordinate mapping.
  - Multi-threaded pyramidal blending for gigapixel mosaic export.

---

## 15. Maintainer Information
- **Author:** Mukul Napa ([GitHub: MukulN7](https://github.com/MukulN7))
- **Organization:** Kips Microanalysis
