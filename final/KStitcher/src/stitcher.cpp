#include "MyStitcher.h"
#include "stitcher.h"

namespace microstitch {

Stitcher::Stitcher(double min_response, double min_spatial, double max_shift)
    : impl_(std::make_unique<StitcherImpl>(min_response, min_spatial, max_shift))
{
}

Stitcher::~Stitcher() = default;

Stitcher::Stitcher(Stitcher&&) noexcept = default;
Stitcher& Stitcher::operator=(Stitcher&&) noexcept = default;

bool Stitcher::processFrame(const cv::Mat& frame) {
    if (!impl_) return false;
    return impl_->processFrame(frame);
}

cv::Mat Stitcher::getMosaic() const {
    if (!impl_) return cv::Mat();
    return impl_->getMosaic();
}

void Stitcher::reset() {
    if (impl_) {
        impl_->reset();
    }
}

bool Stitcher::hasMosaic() const {
    if (!impl_) return false;
    return impl_->hasMosaic();
}

} // namespace microstitch

MICROSTITCH_API void* MicroStitch_Create(double min_response, double min_spatial, double max_shift) {
    try {
        return static_cast<void*>(new microstitch::Stitcher(min_response, min_spatial, max_shift));
    } catch (...) {
        return nullptr;
    }
}

MICROSTITCH_API int MicroStitch_ProcessFrame(
    void* handle,
    const unsigned char* data,
    int width,
    int height,
    int channels,
    int stride
) {
    if (!handle || !data || width <= 0 || height <= 0 || channels <= 0 || stride <= 0) {
        return 0;
    }

    try {
        auto* stitcher = static_cast<microstitch::Stitcher*>(handle);
        int type = (channels == 3) ? CV_8UC3 : CV_8UC1;
        cv::Mat frame(height, width, type, const_cast<unsigned char*>(data), static_cast<size_t>(stride));
        bool ok = stitcher->processFrame(frame);
        return ok ? 1 : 0;
    } catch (...) {
        return -1;
    }
}

MICROSTITCH_API int MicroStitch_GetMosaic(
    void* handle,
    const unsigned char** data,
    int* width,
    int* height,
    int* channels,
    int* stride
) {
    if (!handle || !data || !width || !height || !channels || !stride) {
        return -1;
    }

    try {
        auto* stitcher = static_cast<microstitch::Stitcher*>(handle);
        if (!stitcher->hasMosaic()) {
            *data = nullptr;
            *width = 0;
            *height = 0;
            *channels = 0;
            *stride = 0;
            return 0;
        }

        cv::Mat mosaic = stitcher->getMosaic();
        if (mosaic.empty()) {
            *data = nullptr;
            *width = 0;
            *height = 0;
            *channels = 0;
            *stride = 0;
            return 0;
        }

        // Return dimensions
        *width = mosaic.cols;
        *height = mosaic.rows;
        *channels = mosaic.channels();
        *stride = static_cast<int>(mosaic.step);
        
        // Note: C API callers needing persistent buffer should clone the Mat
        *data = mosaic.data;
        return 1;
    } catch (...) {
        return -1;
    }
}

MICROSTITCH_API void MicroStitch_Reset(void* handle) {
    if (handle) {
        try {
            auto* stitcher = static_cast<microstitch::Stitcher*>(handle);
            stitcher->reset();
        } catch (...) {}
    }
}

MICROSTITCH_API void MicroStitch_Destroy(void* handle) {
    if (handle) {
        try {
            auto* stitcher = static_cast<microstitch::Stitcher*>(handle);
            delete stitcher;
        } catch (...) {}
    }
}

