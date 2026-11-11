/*
 * test_microscope_dll.cpp
 * =======================
 * Native C++ end-to-end integration test for Stitcher.dll with live USB microscope camera.
 *
 * Usage
 * -----
 *   test_microscope_dll.exe <camera_index>
 *
 * Controls
 * --------
 *   Q / ESC  - quit
 *   R        - call Stitcher_Reset()
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <Python.h>

#include "stitcher_api.h"

namespace cv {

struct Mat {
    unsigned char* data;
    int cols;
    int rows;
    int channels_val;
    size_t step;

    Mat() : data(nullptr), cols(0), rows(0), channels_val(0), step(0) {}
    Mat(int r, int c, int type_code, void* ptr, size_t st)
        : data(static_cast<unsigned char*>(ptr)), cols(c), rows(r), channels_val(3), step(st) {}

    int channels() const { return channels_val; }
    bool empty() const { return data == nullptr || cols == 0 || rows == 0; }
};

class VideoCapture {
private:
    PyObject* py_cap;
public:
    VideoCapture() : py_cap(nullptr) {}
    VideoCapture(int index) : py_cap(nullptr) { open(index); }
    ~VideoCapture() { release(); }

    bool open(int index) {
        release();
        PyObject* cv2_mod = PyImport_ImportModule("cv2");
        if (!cv2_mod) return false;
        PyObject* cap_cls = PyObject_GetAttrString(cv2_mod, "VideoCapture");
        Py_DECREF(cv2_mod);
        if (!cap_cls) return false;

        py_cap = PyObject_CallFunction(cap_cls, "i", index);
        Py_DECREF(cap_cls);
        return isOpened();
    }

    bool isOpened() const {
        if (!py_cap) return false;
        PyObject* res = PyObject_CallMethod(py_cap, "isOpened", nullptr);
        bool ok = res && PyObject_IsTrue(res);
        Py_XDECREF(res);
        return ok;
    }

    void set(int propId, double value) {
        if (!py_cap) return;
        PyObject* res = PyObject_CallMethod(py_cap, "set", "id", propId, value);
        Py_XDECREF(res);
    }

    bool read(Mat& frame_out, PyObject** py_frame_owner) {
        if (!py_cap) return false;
        PyObject* res = PyObject_CallMethod(py_cap, "read", nullptr);
        if (!res || !PyTuple_Check(res) || PyTuple_Size(res) < 2) {
            Py_XDECREF(res);
            return false;
        }
        PyObject* ret_flag = PyTuple_GetItem(res, 0);
        PyObject* py_arr = PyTuple_GetItem(res, 1);

        if (!PyObject_IsTrue(ret_flag) || py_arr == Py_None) {
            Py_DECREF(res);
            return false;
        }

        Py_buffer view;
        if (PyObject_GetBuffer(py_arr, &view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) != 0) {
            Py_DECREF(res);
            return false;
        }

        int h = (view.ndim >= 1) ? static_cast<int>(view.shape[0]) : 0;
        int w = (view.ndim >= 2) ? static_cast<int>(view.shape[1]) : 0;
        int ch = (view.ndim >= 3) ? static_cast<int>(view.shape[2]) : 1;
        size_t st = (view.ndim >= 2) ? static_cast<size_t>(view.strides[0]) : w * ch;

        frame_out = Mat(h, w, 16, view.buf, st);
        PyBuffer_Release(&view);

        if (py_frame_owner) {
            Py_INCREF(py_arr);
            *py_frame_owner = py_arr;
        }

        Py_DECREF(res);
        return true;
    }

    void release() {
        if (py_cap) {
            PyObject* res = PyObject_CallMethod(py_cap, "release", nullptr);
            Py_XDECREF(res);
            Py_DECREF(py_cap);
            py_cap = nullptr;
        }
    }
};

inline void imshow(const std::string& winname, const Mat& img) {
    if (img.empty()) return;
    PyObject* cv2_mod = PyImport_ImportModule("cv2");
    if (!cv2_mod) return;

    PyObject* np_mod = PyImport_ImportModule("numpy");
    if (!np_mod) { Py_DECREF(cv2_mod); return; }

    Py_ssize_t nbytes = static_cast<Py_ssize_t>(img.rows) * img.step;
    PyObject* memview = PyMemoryView_FromMemory(
        reinterpret_cast<char*>(img.data), nbytes, PyBUF_READ);
    PyObject* dtype_u8 = PyObject_GetAttrString(np_mod, "uint8");
    PyObject* flat = PyObject_CallMethod(np_mod, "frombuffer", "OO", memview, dtype_u8);
    Py_DECREF(memview);
    Py_XDECREF(dtype_u8);
    Py_DECREF(np_mod);

    if (flat) {
        PyObject* shape = Py_BuildValue("(iii)", img.rows, img.cols, img.channels());
        PyObject* arr = PyObject_CallMethod(flat, "reshape", "O", shape);
        Py_DECREF(flat);
        Py_DECREF(shape);

        if (arr) {
            PyObject* res = PyObject_CallMethod(cv2_mod, "imshow", "sO", winname.c_str(), arr);
            Py_XDECREF(res);
            Py_DECREF(arr);
        }
    }
    Py_DECREF(cv2_mod);
}

inline int waitKey(int delay = 1) {
    PyObject* cv2_mod = PyImport_ImportModule("cv2");
    if (!cv2_mod) return -1;
    PyObject* res = PyObject_CallMethod(cv2_mod, "waitKey", "i", delay);
    int key = -1;
    if (res) {
        key = static_cast<int>(PyLong_AsLong(res)) & 0xFF;
        Py_DECREF(res);
    }
    Py_DECREF(cv2_mod);
    return key;
}

inline void destroyAllWindows() {
    PyObject* cv2_mod = PyImport_ImportModule("cv2");
    if (!cv2_mod) return;
    PyObject* res = PyObject_CallMethod(cv2_mod, "destroyAllWindows", nullptr);
    Py_XDECREF(res);
    Py_DECREF(cv2_mod);
}

} // namespace cv

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: test_microscope_dll.exe <camera_index>" << std::endl;
        return 1;
    }

    int camera_index = std::atoi(argv[1]);

    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    std::cout << "[INFO] Opening camera index " << camera_index << "..." << std::endl;
    cv::VideoCapture cap(camera_index);
    if (!cap.isOpened()) {
        std::cerr << "[ERROR] Cannot open camera index " << camera_index << std::endl;
        return 1;
    }

    // CAP_PROP_FRAME_WIDTH = 3, CAP_PROP_FRAME_HEIGHT = 4
    cap.set(3, 640.0);
    cap.set(4, 360.0);

    std::cout << "[INFO] Creating Stitcher instance (640x360, 3 channels)..." << std::endl;
    void* handle = Stitcher_Create(640, 360, 3);
    if (!handle) {
        std::cerr << "[ERROR] Stitcher_Create failed!" << std::endl;
        return 1;
    }

    std::cout << "[INFO] Camera opened. Controls: Q/ESC = quit, R = reset" << std::endl;

    int frame_count = 0;
    while (true) {
        cv::Mat frame;
        PyObject* py_frame_owner = nullptr;
        if (!cap.read(frame, &py_frame_owner)) {
            continue;
        }

        frame_count++;

        int rc = Stitcher_ProcessFrame(
            handle,
            frame.data,
            frame.cols,
            frame.rows,
            frame.channels(),
            static_cast<int>(frame.step)
        );

        const unsigned char* mosaic_bytes = nullptr;
        int mw = 0, mh = 0, mc = 0, ms = 0;
        int has_mosaic = Stitcher_GetMosaic(handle, &mosaic_bytes, &mw, &mh, &mc, &ms);

        if (frame_count % 30 == 0) {
            std::cout << "[Frame " << frame_count << "] ProcessFrame rc=" << rc
                      << "  Mosaic: " << (has_mosaic ? (std::to_string(mw) + "x" + std::to_string(mh)) : "None")
                      << std::endl;
        }

        cv::imshow("Camera", frame);

        if (has_mosaic && mosaic_bytes != nullptr) {
            cv::Mat mosaic_mat(mh, mw, 16, const_cast<unsigned char*>(mosaic_bytes), static_cast<size_t>(ms));
            cv::imshow("DLL Mosaic", mosaic_mat);
        }

        Py_XDECREF(py_frame_owner);

        int key = cv::waitKey(1);
        if (key == 'q' || key == 'Q' || key == 27) {
            std::cout << "[INFO] Quitting..." << std::endl;
            break;
        } else if (key == 'r' || key == 'R') {
            Stitcher_Reset(handle);
            std::cout << "[INFO] Stitcher_Reset() called at frame " << frame_count << std::endl;
        }
    }

    std::cout << "[INFO] Cleaning up..." << std::endl;
    Stitcher_Destroy(handle);
    cap.release();
    cv::destroyAllWindows();
    std::cout << "[INFO] Finished cleanly." << std::endl;

    return 0;
}
