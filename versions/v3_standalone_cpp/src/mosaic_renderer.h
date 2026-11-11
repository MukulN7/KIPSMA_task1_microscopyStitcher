#pragma once

#include <opencv2/opencv.hpp>
#include "mosaic.h"

namespace microstitch {

class MosaicCanvasRenderer {
public:
    MosaicCanvasRenderer() = default;

    cv::Mat render(const MosaicManager& manager, bool blend = true);

    static cv::Mat generateWeightMask(int h, int w);
};

} // namespace microstitch
