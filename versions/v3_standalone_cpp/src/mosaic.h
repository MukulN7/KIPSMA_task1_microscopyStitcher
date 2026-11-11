#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include "registration.h"

namespace microstitch {

struct MosaicTile {
    int frame_index = 0;
    double global_x = 0.0;
    double global_y = 0.0;
    int width = 0;
    int height = 0;
    int type = CV_8UC3;
    double quality_score = 0.0;
    double response = 0.0;
    bool is_origin = false;
    cv::Mat frame;
};

class MosaicManager {
public:
    MosaicManager();

    void reset();

    bool addFirstFrame(const cv::Mat& frame, MosaicTile* out_tile = nullptr);
    bool addFrame(const cv::Mat& frame, const RegistrationResult& reg_result, MosaicTile* out_tile = nullptr);

    const std::vector<MosaicTile>& getTiles() const { return tiles_; }

    void getGlobalBounds(double& min_x, double& min_y, double& max_x, double& max_y) const;

    int tileCount() const { return static_cast<int>(tiles_.size()); }

    void currentPosition(double& x, double& y) const {
        x = current_x_;
        y = current_y_;
    }

private:
    std::vector<MosaicTile> tiles_;
    double current_x_ = 0.0;
    double current_y_ = 0.0;
};

} // namespace microstitch
