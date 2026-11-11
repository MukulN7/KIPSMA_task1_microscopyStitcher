# MicroStitch v2 - C-Compatible Win32 DLL Integration Wrapper

## Overview
This directory preserves the hybrid integration architecture where the core Python stitching engine was packaged inside **`Stitcher.dll`**, a pure C-compatible Win32 dynamic link library, enabling an MFC (Microsoft Foundation Classes) C++ desktop host application to execute the stitching pipeline without exposing CPython or NumPy types across the DLL boundary.

## Contents
- `native/`:
  - `include/stitcher_api.h`: Plain C exported API (`microstitch_create`, `microstitch_process_frame`, `microstitch_get_mosaic`, `microstitch_reset`, `microstitch_destroy`).
  - `bin/Stitcher.dll` & `Stitcher.lib`: Compiled 64-bit DLL and import library wrapping the embedded CPython runtime.
- `microstitch/`: Python backend engine containing registration, mosaic canvas management, and distance-weighted blending.
- `MFC_STITCHER_HANDOFF.md`: Comprehensive handoff guide for MFC application developers, detailing memory layouts, thread safety (`PyGILState`), and API usage.
- `scratch_test_engine_camera.py`: Diagnostic script testing camera capture directly against the engine interface.
- `requirements.txt`: Python runtime dependencies for the embedded interpreter environment.

## Key Changes from v1
- Introduction of the C API boundary (`stitcher_api.h`) with raw BGR buffer passing.
- Thread-safe acquisition of the Python Global Interpreter Lock (GIL).
- Direct host-driven camera frame injection.

## Status
- **Status**: Intermediate integration milestone. Replaced by pure C++17 implementations (v3 & final) to eliminate Python runtime and DLL deployment overhead.
