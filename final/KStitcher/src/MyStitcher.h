#pragma once

#include <opencv2/opencv.hpp>
#include <memory>


#define MICROSTITCH_API


namespace microstitch {

class StitcherImpl;

class Stitcher {
public:
    Stitcher(
        double min_response = 0.05,
        double min_spatial = 0.35,
        double max_shift = 150.0
    );

    ~Stitcher();

    Stitcher(const Stitcher&) = delete;
    Stitcher& operator=(const Stitcher&) = delete;
    Stitcher(Stitcher&&) noexcept;
    Stitcher& operator=(Stitcher&&) noexcept;

    bool processFrame(const cv::Mat& frame);

    cv::Mat getMosaic() const;

    void reset();

    bool hasMosaic() const;

private:
    std::unique_ptr<StitcherImpl> impl_;
};

} // namespace microstitch

MICROSTITCH_API void* MicroStitch_Create(
    double min_response,
    double min_spatial,
    double max_shift
);

MICROSTITCH_API int MicroStitch_ProcessFrame(
    void* handle,
    const unsigned char* data,
    int width,
    int height,
    int channels,
    int stride
);

MICROSTITCH_API int MicroStitch_GetMosaic(
    void* handle,
    const unsigned char** data,
    int* width,
    int* height,
    int* channels,
    int* stride
);

MICROSTITCH_API void MicroStitch_Reset(
    void* handle
);

MICROSTITCH_API void MicroStitch_Destroy(
    void* handle
);


