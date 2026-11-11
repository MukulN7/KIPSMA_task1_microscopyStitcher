/*
 * stitcher_api.cpp
 * ================
 * Full CPython-embedding implementation of the Stitcher DLL C API.
 *
 * Architecture
 * ------------
 *   MFC caller  →  C API (stitcher_api.h)
 *                      ↓
 *               StitcherHandle  (opaque void* seen by MFC)
 *                      ↓
 *               Embedded CPython  (Py_Initialize / Python C API)
 *                      ↓
 *               microstitch.engine.MicroStitchEngine  (Python)
 *                      ↓
 *               RegistrationEngine / MosaicManager / MosaicCanvasRenderer
 *
 * Key design decisions
 * --------------------
 * - One persistent MicroStitchEngine Python object per handle.
 * - Incoming raw bytes are wrapped in a numpy array WITHOUT copying using
 *   PyMemoryView → numpy.frombuffer, then reshaped to (H, W, C).
 * - The mosaic returned by process_frame() is kept alive as a PyObject*
 *   ref inside the handle.  GetMosaic() hands the MFC caller a direct
 *   pointer into that array's data buffer; no extra copy is made.
 * - Thread Safety: All Python C API calls are guarded with
 *   PyGILState_Ensure() and PyGILState_Release() to ensure valid thread
 *   state whether called from native C++ (MFC) or Python (ctypes).
 * - Python exceptions never cross the DLL boundary: they are printed to
 *   stderr and translated to integer error codes.
 * - The global Python interpreter is NOT shut down in Stitcher_Destroy
 *   because other handles (or the host application itself) might still use
 *   it.  Py_Finalize() is intentionally omitted.
 */

#include "stitcher_api.h"

/* Python.h must be included before any standard headers on Windows. */
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

/* =========================================================================
 * Internal handle
 * ====================================================================== */
struct StitcherHandle {
    int  expected_width;
    int  expected_height;
    int  expected_channels;

    PyObject* py_engine;       /* microstitch.engine.MicroStitchEngine instance */
    PyObject* py_mosaic;       /* most-recently accepted mosaic ndarray, or NULL */

    /* Raw pointer into py_mosaic's data buffer (valid as long as py_mosaic
     * is alive and not reallocated by NumPy).  Handed to MFC via GetMosaic. */
    const unsigned char* mosaic_data;
    int  mosaic_width;
    int  mosaic_height;
    int  mosaic_channels;
    int  mosaic_stride;
};

/* =========================================================================
 * Python helper utilities
 * ====================================================================== */

/* Print and clear any pending Python exception to stderr.
 * Returns true if an exception was active (and has now been cleared). */
static bool _check_and_print_exception(const char* ctx)
{
    if (!PyErr_Occurred()) return false;
    fprintf(stderr, "[Stitcher.dll] Python exception in %s:\n", ctx);
    PyErr_Print();          /* prints traceback; clears the exception */
    fflush(stderr);
    return true;
}

/* Ensure the Python interpreter is initialised.
 * Safe to call multiple times. */
static bool _ensure_python()
{
    if (Py_IsInitialized()) return true;
    Py_Initialize();
    if (!Py_IsInitialized()) {
        fprintf(stderr, "[Stitcher.dll] Py_Initialize() failed.\n");
        return false;
    }
    return true;
}

/* Prepend project root to sys.path so microstitch package can be imported. */
static bool _append_project_root_to_syspath()
{
    PyObject* sys_module = PyImport_ImportModule("sys");
    if (!sys_module) { _check_and_print_exception("_append_project_root_to_syspath/import sys"); return false; }

    PyObject* sys_path = PyObject_GetAttrString(sys_module, "path");
    Py_DECREF(sys_module);
    if (!sys_path) { _check_and_print_exception("_append_project_root_to_syspath/sys.path"); return false; }

    const char* env_root = getenv("STITCHER_PROJECT_ROOT");
    PyObject* py_root = nullptr;

    if (env_root && env_root[0]) {
        py_root = PyUnicode_DecodeFSDefault(env_root);
    } else {
        py_root = PyUnicode_FromString("");
    }

    if (!py_root) {
        Py_DECREF(sys_path);
        _check_and_print_exception("_append_project_root_to_syspath/PyUnicode");
        return false;
    }

    int rc = PyList_Insert(sys_path, 0, py_root);
    Py_DECREF(py_root);
    Py_DECREF(sys_path);

    if (rc != 0) {
        _check_and_print_exception("_append_project_root_to_syspath/insert");
        return false;
    }
    return true;
}

/* =========================================================================
 * Stitcher_Create
 * ====================================================================== */
void* Stitcher_Create(int width, int height, int channels)
{
    if (!_ensure_python()) return nullptr;

    PyGILState_STATE gstate = PyGILState_Ensure();

    if (!_append_project_root_to_syspath()) {
        PyGILState_Release(gstate);
        return nullptr;
    }

    /* Import microstitch.engine */
    PyObject* mod = PyImport_ImportModule("microstitch.engine");
    if (!mod) {
        _check_and_print_exception("Stitcher_Create/import microstitch.engine");
        PyGILState_Release(gstate);
        return nullptr;
    }

    /* Get the MicroStitchEngine class */
    PyObject* cls = PyObject_GetAttrString(mod, "MicroStitchEngine");
    Py_DECREF(mod);
    if (!cls) {
        _check_and_print_exception("Stitcher_Create/MicroStitchEngine class");
        PyGILState_Release(gstate);
        return nullptr;
    }

    /* Instantiate:  MicroStitchEngine() */
    PyObject* engine = PyObject_CallNoArgs(cls);
    Py_DECREF(cls);
    if (!engine) {
        _check_and_print_exception("Stitcher_Create/MicroStitchEngine()");
        PyGILState_Release(gstate);
        return nullptr;
    }

    /* Allocate the C handle */
    StitcherHandle* h = static_cast<StitcherHandle*>(malloc(sizeof(StitcherHandle)));
    if (!h) {
        Py_DECREF(engine);
        PyGILState_Release(gstate);
        return nullptr;
    }
    memset(h, 0, sizeof(StitcherHandle));
    h->expected_width    = width;
    h->expected_height   = height;
    h->expected_channels = channels;
    h->py_engine         = engine;   /* owned reference */
    h->py_mosaic         = nullptr;
    h->mosaic_data       = nullptr;

    PyGILState_Release(gstate);
    return static_cast<void*>(h);
}

/* =========================================================================
 * Stitcher_ProcessFrame
 * ====================================================================== */
int Stitcher_ProcessFrame(
    void*                handle,
    const unsigned char* data,
    int                  width,
    int                  height,
    int                  channels,
    int                  stride)
{
    if (!handle || !data)                                     return 0;
    if (width <= 0 || height <= 0 || channels <= 0 || stride <= 0) return 0;

    StitcherHandle* h = static_cast<StitcherHandle*>(handle);

    PyGILState_STATE gstate = PyGILState_Ensure();

    const int tight_stride = width * channels;
    PyObject* py_frame     = nullptr;

    if (stride == tight_stride) {
        /* Zero-copy path */
        Py_ssize_t nbytes = static_cast<Py_ssize_t>(height) * stride;
        PyObject* memview = PyMemoryView_FromMemory(
            reinterpret_cast<char*>(const_cast<unsigned char*>(data)),
            nbytes,
            PyBUF_READ);
        if (!memview) {
            _check_and_print_exception("ProcessFrame/PyMemoryView_FromMemory");
            PyGILState_Release(gstate);
            return -1;
        }

        PyObject* np_mod = PyImport_ImportModule("numpy");
        if (!np_mod) { Py_DECREF(memview); _check_and_print_exception("ProcessFrame/import numpy"); PyGILState_Release(gstate); return -1; }

        PyObject* dtype_uint8 = PyObject_GetAttrString(np_mod, "uint8");
        PyObject* flat = PyObject_CallMethod(np_mod, "frombuffer", "OO", memview, dtype_uint8);
        Py_DECREF(memview);
        Py_XDECREF(dtype_uint8);
        Py_DECREF(np_mod);
        if (!flat) { _check_and_print_exception("ProcessFrame/frombuffer"); PyGILState_Release(gstate); return -1; }

        PyObject* shape = Py_BuildValue("(iii)", height, width, channels);
        PyObject* reshaped = PyObject_CallMethod(flat, "reshape", "O", shape);
        Py_DECREF(flat);
        Py_DECREF(shape);
        if (!reshaped) { _check_and_print_exception("ProcessFrame/reshape"); PyGILState_Release(gstate); return -1; }

        /* Create an independent owned copy of the array so py_frame persists safely
         * in MosaicManager when the C++ host reuses or releases its frame buffer. */
        py_frame = PyObject_CallMethod(reshaped, "copy", nullptr);
        Py_DECREF(reshaped);
        if (!py_frame) { _check_and_print_exception("ProcessFrame/copy"); PyGILState_Release(gstate); return -1; }

    } else {
        /* Strided path */
        Py_ssize_t nbytes = static_cast<Py_ssize_t>(height) * tight_stride;
        unsigned char* buf = static_cast<unsigned char*>(malloc(nbytes));
        if (!buf) { PyGILState_Release(gstate); return -1; }

        for (int row = 0; row < height; ++row) {
            memcpy(buf + row * tight_stride,
                   data + row * stride,
                   static_cast<size_t>(tight_stride));
        }

        PyObject* np_mod = PyImport_ImportModule("numpy");
        if (!np_mod) { free(buf); PyGILState_Release(gstate); return -1; }

        PyObject* py_bytes = PyBytes_FromStringAndSize(
            reinterpret_cast<const char*>(buf), nbytes);
        free(buf);
        if (!py_bytes) { Py_DECREF(np_mod); _check_and_print_exception("ProcessFrame/PyBytes"); PyGILState_Release(gstate); return -1; }

        PyObject* dtype_uint8 = PyObject_GetAttrString(np_mod, "uint8");
        PyObject* flat = PyObject_CallMethod(np_mod, "frombuffer", "OO", py_bytes, dtype_uint8);
        Py_DECREF(py_bytes);
        Py_XDECREF(dtype_uint8);
        Py_DECREF(np_mod);
        if (!flat) { _check_and_print_exception("ProcessFrame/frombuffer(strided)"); PyGILState_Release(gstate); return -1; }

        PyObject* shape = Py_BuildValue("(iii)", height, width, channels);
        PyObject* reshaped = PyObject_CallMethod(flat, "reshape", "O", shape);
        Py_DECREF(flat);
        Py_DECREF(shape);
        if (!reshaped) { _check_and_print_exception("ProcessFrame/reshape(strided)"); PyGILState_Release(gstate); return -1; }

        py_frame = PyObject_CallMethod(reshaped, "copy", nullptr);
        Py_DECREF(reshaped);
        if (!py_frame) { _check_and_print_exception("ProcessFrame/copy(strided)"); PyGILState_Release(gstate); return -1; }
    }

    /* Call engine.process_frame(frame) */
    PyObject* result = PyObject_CallMethod(h->py_engine, "process_frame", "O", py_frame);
    Py_DECREF(py_frame);
    if (!result) {
        _check_and_print_exception("ProcessFrame/process_frame()");
        PyGILState_Release(gstate);
        return -1;
    }

    /* Extract "mosaic" */
    PyObject* py_mosaic = PyDict_GetItemString(result, "mosaic");  /* borrowed ref */
    if (py_mosaic && py_mosaic != Py_None) {
        Py_XDECREF(h->py_mosaic);
        Py_INCREF(py_mosaic);
        h->py_mosaic = py_mosaic;

        Py_buffer view;
        if (PyObject_GetBuffer(h->py_mosaic, &view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == 0) {
            h->mosaic_data     = static_cast<const unsigned char*>(view.buf);
            h->mosaic_height   = (view.ndim >= 1) ? static_cast<int>(view.shape[0]) : 0;
            h->mosaic_width    = (view.ndim >= 2) ? static_cast<int>(view.shape[1]) : 0;
            h->mosaic_channels = (view.ndim >= 3) ? static_cast<int>(view.shape[2]) : 1;
            h->mosaic_stride   = (view.ndim >= 2)
                                     ? static_cast<int>(view.strides[0])
                                     : h->mosaic_width * h->mosaic_channels;
            PyBuffer_Release(&view);
        } else {
            _check_and_print_exception("ProcessFrame/GetBuffer(mosaic)");
            h->mosaic_data = nullptr;
        }
    }

    PyObject* py_valid = PyDict_GetItemString(result, "registration_valid");
    int valid = (py_valid && PyObject_IsTrue(py_valid)) ? 1 : 0;
    Py_DECREF(result);

    PyGILState_Release(gstate);
    return valid;
}

/* =========================================================================
 * Stitcher_GetMosaic
 * ====================================================================== */
int Stitcher_GetMosaic(
    void*                 handle,
    const unsigned char** data,
    int*                  width,
    int*                  height,
    int*                  channels,
    int*                  stride)
{
    if (!handle || !data || !width || !height || !channels || !stride)
        return -1;

    StitcherHandle* h = static_cast<StitcherHandle*>(handle);

    if (!h->py_mosaic || !h->mosaic_data) {
        *data     = nullptr;
        *width    = 0;
        *height   = 0;
        *channels = 0;
        *stride   = 0;
        return 0;
    }

    *data     = h->mosaic_data;
    *width    = h->mosaic_width;
    *height   = h->mosaic_height;
    *channels = h->mosaic_channels;
    *stride   = h->mosaic_stride;
    return 1;
}

/* =========================================================================
 * Stitcher_Reset
 * ====================================================================== */
void Stitcher_Reset(void* handle)
{
    if (!handle) return;
    StitcherHandle* h = static_cast<StitcherHandle*>(handle);

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* ret = PyObject_CallMethod(h->py_engine, "reset", nullptr);
    if (!ret) {
        _check_and_print_exception("Stitcher_Reset/reset()");
    } else {
        Py_DECREF(ret);
    }

    Py_XDECREF(h->py_mosaic);
    h->py_mosaic     = nullptr;
    h->mosaic_data   = nullptr;
    h->mosaic_width  = 0;
    h->mosaic_height = 0;
    h->mosaic_channels = 0;
    h->mosaic_stride = 0;

    PyGILState_Release(gstate);
}

/* =========================================================================
 * Stitcher_Destroy
 * ====================================================================== */
void Stitcher_Destroy(void* handle)
{
    if (!handle) return;
    StitcherHandle* h = static_cast<StitcherHandle*>(handle);

    PyGILState_STATE gstate = PyGILState_Ensure();

    Py_XDECREF(h->py_mosaic);
    Py_XDECREF(h->py_engine);

    PyGILState_Release(gstate);

    free(h);
}
