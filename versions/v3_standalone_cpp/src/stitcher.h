#pragma once

#include "microstitch/stitcher.h"
#include "registration.h"
#include "mosaic.h"
#include "mosaic_renderer.h"

namespace microstitch {

class StitcherImpl {
public:
    StitcherImpl(double min_response, double min_spatial, double max_shift)
        : engine_(min_response, min_spatial, max_shift, true, true, min_response, true)
    {
    }

    bool processFrame(const cv::Mat& frame) {
        if (frame.empty()) {
            return false;
        }

        if (prev_frame_.empty()) {
            prev_frame_ = frame.clone();
            mosaic_.addFirstFrame(frame);
            cached_mosaic_ = renderer_.render(mosaic_, true);
            return true;
        }

        cv::Mat ref_frame = prev_frame_;
        prev_frame_ = frame.clone();

        RegistrationResult reg = engine_.registerFrames(ref_frame, frame);
        if (reg.valid) {
            mosaic_.addFrame(frame, reg);
            cached_mosaic_ = renderer_.render(mosaic_, true);
            return true;
        }

        return false;
    }

    cv::Mat getMosaic() const {
        return cached_mosaic_.clone();
    }

    const cv::Mat& getMosaicRef() const {
        return cached_mosaic_;
    }

    void reset() {
        mosaic_.reset();
        prev_frame_.release();
        cached_mosaic_.release();
    }

    bool hasMosaic() const {
        return mosaic_.tileCount() > 0;
    }

private:
    RegistrationEngine engine_;
    MosaicManager mosaic_;
    MosaicCanvasRenderer renderer_;
    cv::Mat prev_frame_;
    cv::Mat cached_mosaic_;
};

} // namespace microstitch
