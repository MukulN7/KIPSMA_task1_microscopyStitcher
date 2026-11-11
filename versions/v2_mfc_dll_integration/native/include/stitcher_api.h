/*
 * stitcher_api.h
 * ==============
 * C-compatible public API for Stitcher.dll.
 *
 * Designed to be called from MFC (or any C/C++ host) without exposing any
 * Python, NumPy, or OpenCV types across the DLL boundary.
 *
 * All image data is passed and returned as raw byte buffers so that the
 * host application (MFC) can manage its own memory independently.
 *
 * Usage pattern
 * -------------
 *   void* h = Stitcher_Create(640, 360, 3);
 *   while (capturing) {
 *       int ok = Stitcher_ProcessFrame(h, pixels, w, h, ch, stride);
 *       if (ok) {
 *           const unsigned char* mos;  int mw, mh, mc, ms;
 *           Stitcher_GetMosaic(h, &mos, &mw, &mh, &mc, &ms);
 *           // copy / display mos
 *       }
 *   }
 *   Stitcher_Reset(h);    // optional mid-session reset
 *   Stitcher_Destroy(h);
 */

#pragma once

#ifdef _WIN32
  #ifdef STITCHER_EXPORTS
    /* Building the DLL — export symbols */
    #define STITCHER_API __declspec(dllexport)
  #else
    /* Consuming the DLL — import symbols */
    #define STITCHER_API __declspec(dllimport)
  #endif
#else
  /* Non-Windows: no decoration needed */
  #define STITCHER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Stitcher_Create
 * ---------------
 * Allocates and initialises a persistent stitcher instance.
 *
 * Parameters
 *   width    – expected frame width  in pixels (informational; the engine
 *              adapts to the actual frame dimensions passed to ProcessFrame)
 *   height   – expected frame height in pixels
 *   channels – number of colour channels (1 = grayscale, 3 = BGR)
 *
 * Returns
 *   Opaque handle to the stitcher instance, or NULL on failure.
 *   The caller must eventually pass this handle to Stitcher_Destroy().
 * ---------------------------------------------------------------------- */
STITCHER_API void* Stitcher_Create(
    int width,
    int height,
    int channels
);

/* -------------------------------------------------------------------------
 * Stitcher_ProcessFrame
 * ---------------------
 * Feeds one raw image frame into the stitching pipeline.
 *
 * The DLL copies the pixel data immediately; the caller may free or reuse
 * the buffer as soon as this function returns.
 *
 * Parameters
 *   handle   – opaque handle returned by Stitcher_Create()
 *   data     – pointer to the first byte of the image buffer
 *              (row-major, top-left origin, packed as BGR or grayscale)
 *   width    – frame width  in pixels
 *   height   – frame height in pixels
 *   channels – number of colour channels (must match Stitcher_Create)
 *   stride   – number of bytes per row (>= width * channels);
 *              pass (width * channels) if the buffer is tightly packed
 *
 * Returns
 *   1  if the frame was accepted and added to the mosaic
 *   0  if the frame was rejected (registration failed quality gates)
 *  -1  on error (NULL handle, NULL data, or internal failure)
 * ---------------------------------------------------------------------- */
STITCHER_API int Stitcher_ProcessFrame(
    void*                handle,
    const unsigned char* data,
    int                  width,
    int                  height,
    int                  channels,
    int                  stride
);

/* -------------------------------------------------------------------------
 * Stitcher_GetMosaic
 * ------------------
 * Returns a pointer to the DLL-owned mosaic pixel buffer and its dimensions.
 *
 * The returned pointer is valid until the next call to Stitcher_ProcessFrame,
 * Stitcher_Reset, or Stitcher_Destroy on the same handle.
 * The caller must NOT free the returned buffer.
 *
 * Parameters
 *   handle   – opaque handle returned by Stitcher_Create()
 *   data     – [out] pointer to the mosaic byte buffer, or NULL if no tiles
 *   width    – [out] mosaic canvas width  in pixels
 *   height   – [out] mosaic canvas height in pixels
 *   channels – [out] number of colour channels in the mosaic
 *   stride   – [out] number of bytes per row in the mosaic buffer
 *
 * Returns
 *   1  if a valid mosaic buffer is available (*data != NULL)
 *   0  if no frames have been processed yet (*data == NULL)
 *  -1  on error (NULL handle or NULL output pointers)
 * ---------------------------------------------------------------------- */
STITCHER_API int Stitcher_GetMosaic(
    void*                 handle,
    const unsigned char** data,
    int*                  width,
    int*                  height,
    int*                  channels,
    int*                  stride
);

/* -------------------------------------------------------------------------
 * Stitcher_Reset
 * --------------
 * Clears all accumulated mosaic tiles and resets the internal stitcher
 * state.  The instance remains valid and can accept new frames immediately
 * after this call.
 *
 * Parameters
 *   handle – opaque handle returned by Stitcher_Create()
 * ---------------------------------------------------------------------- */
STITCHER_API void Stitcher_Reset(
    void* handle
);

/* -------------------------------------------------------------------------
 * Stitcher_Destroy
 * ----------------
 * Releases all resources held by the stitcher instance (Python objects,
 * mosaic buffer, embedded interpreter references).
 *
 * The handle is invalid after this call; passing it to any other function
 * results in undefined behaviour.
 *
 * Parameters
 *   handle – opaque handle returned by Stitcher_Create()
 * ---------------------------------------------------------------------- */
STITCHER_API void Stitcher_Destroy(
    void* handle
);

#ifdef __cplusplus
} /* extern "C" */
#endif
