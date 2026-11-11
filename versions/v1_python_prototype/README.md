# MicroStitch v1 - Python Prototype & Algorithmic Exploration

## Overview
This directory preserves the initial research and development prototype of the **MicroStitch** microscope image-stitching pipeline. Built in Python 3.10+ using OpenCV (`cv2`) and NumPy, it demonstrated real-time translation estimation, phase correlation, and dynamic mosaic rendering from live or simulated microscope video feeds.

## Contents
- `microstitch/`:
  - `movement_detector.py`: Lightweight optical difference detector to prevent processing stationary or jittery frames.
  - `registration.py`: Rigid 2D translation estimator using Fourier phase correlation (`cv2.phaseCorrelate`) and SIFT feature matching with RANSAC.
  - `mosaic.py`: Global coordinate accumulator and canvas expansion manager.
  - `mosaic_renderer.py`: Seam-free multi-band / linear distance feathering blender.
  - `live_stitcher.py`: Orchestrator connecting camera capture, registration, and visualization.
  - `frame_source.py`: Video capture abstraction supporting USB webcams, industrial microscope cameras, and disk-based image sequences.
- `tests/`: Automated test suite for movement detection, registration, and blending logic.
- `PROJECT_CONTEXT.md`: Complete original specification document defining physical system constraints, motion models, and technology decisions.

## Technical Approach & Status
- **Motion Model**: Strict 2D Rigid Translation ($\Delta x, \Delta y$) under fixed optical focal length and perpendicular slide motion.
- **Status**: Historical algorithmic prototype. Superseded by the native C++ standalone engine (v3) and the MFC desktop application (`final/`).
