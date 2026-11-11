#include "stdafx.h"
#include "KStitcher.h"

#include "mosaic.h"
#include <algorithm>
#include <limits>

namespace microstitch {

MosaicManager::MosaicManager()
    : current_x_(0.0), current_y_(0.0)
{
}

void MosaicManager::reset() {
    tiles_.clear();
    current_x_ = 0.0;
    current_y_ = 0.0;
}

bool MosaicManager::addFirstFrame(const cv::Mat& frame, MosaicTile* out_tile) {
    if (frame.empty()) {
        return false;
    }

    reset();

    MosaicTile tile;
    tile.frame_index = 0;
    tile.global_x = 0.0;
    tile.global_y = 0.0;
    tile.width = frame.cols;
    tile.height = frame.rows;
    tile.type = frame.type();
    tile.quality_score = 1.0;
    tile.response = 1.0;
    tile.is_origin = true;
    tile.frame = frame.clone();

    tiles_.push_back(tile);

    if (out_tile) {
        *out_tile = tile;
    }
    return true;
}

bool MosaicManager::addFrame(const cv::Mat& frame, const RegistrationResult& reg_result, MosaicTile* out_tile) {
    if (frame.empty() || !reg_result.valid) {
        return false;
    }

    if (tiles_.empty()) {
        return addFirstFrame(frame, out_tile);
    }

    current_x_ -= reg_result.dx;
    current_y_ -= reg_result.dy;

    MosaicTile tile;
    tile.frame_index = static_cast<int>(tiles_.size());
    tile.global_x = current_x_;
    tile.global_y = current_y_;
    tile.width = frame.cols;
    tile.height = frame.rows;
    tile.type = frame.type();
    tile.quality_score = reg_result.quality_score;
    tile.response = reg_result.response;
    tile.is_origin = false;
    tile.frame = frame.clone();

    tiles_.push_back(tile);

    if (out_tile) {
        *out_tile = tile;
    }
    return true;
}

void MosaicManager::getGlobalBounds(double& min_x, double& min_y, double& max_x, double& max_y) const {
    if (tiles_.empty()) {
        min_x = 0.0;
        min_y = 0.0;
        max_x = 0.0;
        max_y = 0.0;
        return;
    }

    min_x = std::numeric_limits<double>::infinity();
    min_y = std::numeric_limits<double>::infinity();
    max_x = -std::numeric_limits<double>::infinity();
    max_y = -std::numeric_limits<double>::infinity();

    for (const auto& tile : tiles_) {
        double x1 = tile.global_x;
        double y1 = tile.global_y;
        double x2 = x1 + tile.width;
        double y2 = y1 + tile.height;

        min_x = std::min(min_x, x1);
        min_y = std::min(min_y, y1);
        max_x = std::max(max_x, x2);
        max_y = std::max(max_y, y2);
    }
}

} // namespace microstitch
