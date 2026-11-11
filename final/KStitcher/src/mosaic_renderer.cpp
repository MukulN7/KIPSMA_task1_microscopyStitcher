#include "mosaic_renderer.h"
#include <cmath>
#include <algorithm>

namespace microstitch {

cv::Mat MosaicCanvasRenderer::generateWeightMask(int h, int w) {
    cv::Mat mask(h, w, CV_32FC1);
    for (int y = 0; y < h; ++y) {
        float dy = static_cast<float>(std::min(y, h - 1 - y) + 1);
        float* ptr = mask.ptr<float>(y);
        for (int x = 0; x < w; ++x) {
            float dx = static_cast<float>(std::min(x, w - 1 - x) + 1);
            ptr[x] = std::min(dx, dy);
        }
    }
    return mask;
}

cv::Mat MosaicCanvasRenderer::render(const MosaicManager& manager, bool blend) {
    const auto& tiles = manager.getTiles();
    if (tiles.empty()) {
        return cv::Mat();
    }

    double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
    manager.getGlobalBounds(min_x, min_y, max_x, max_y);

    int canvas_width = static_cast<int>(std::ceil(max_x - min_x));
    int canvas_height = static_cast<int>(std::ceil(max_y - min_y));

    if (canvas_width <= 0 || canvas_height <= 0) {
        return cv::Mat();
    }

    int target_type = tiles[0].frame.type();
    int channels = tiles[0].frame.channels();

    if (!blend) {
        // Deterministic Overwrite mode
        cv::Mat canvas = cv::Mat::zeros(canvas_height, canvas_width, target_type);
        for (const auto& tile : tiles) {
            if (tile.frame.empty()) continue;

            int col_start = static_cast<int>(std::round(tile.global_x - min_x));
            int row_start = static_cast<int>(std::round(tile.global_y - min_y));
            int h = tile.height;
            int w = tile.width;

            int r1 = std::max(0, row_start);
            int r2 = std::min(canvas_height, row_start + h);
            int c1 = std::max(0, col_start);
            int c2 = std::min(canvas_width, col_start + w);

            if (r2 <= r1 || c2 <= c1) continue;

            int fr1 = r1 - row_start;
            int fc1 = c1 - col_start;

            tile.frame(cv::Rect(fc1, fr1, c2 - c1, r2 - r1))
                .copyTo(canvas(cv::Rect(c1, r1, c2 - c1, r2 - r1)));
        }
        return canvas;
    }

    // Weighted Accumulation Blending mode
    cv::Mat accum_canvas = cv::Mat::zeros(canvas_height, canvas_width, CV_32FC(channels));
    cv::Mat weight_canvas = cv::Mat::zeros(canvas_height, canvas_width, CV_32FC(channels));

    for (const auto& tile : tiles) {
        if (tile.frame.empty()) continue;

        int col_start = static_cast<int>(std::round(tile.global_x - min_x));
        int row_start = static_cast<int>(std::round(tile.global_y - min_y));
        int h = tile.height;
        int w = tile.width;

        int r1 = std::max(0, row_start);
        int r2 = std::min(canvas_height, row_start + h);
        int c1 = std::max(0, col_start);
        int c2 = std::min(canvas_width, col_start + w);

        if (r2 <= r1 || c2 <= c1) continue;

        int fr1 = r1 - row_start;
        int fc1 = c1 - col_start;

        cv::Mat weight_2d = generateWeightMask(h, w);
        cv::Mat crop_weight = weight_2d(cv::Rect(fc1, fr1, c2 - c1, r2 - r1));
        cv::Mat crop_frame = tile.frame(cv::Rect(fc1, fr1, c2 - c1, r2 - r1));

        if (channels == 3) {
            for (int y = 0; y < r2 - r1; ++y) {
                const float* w_ptr = crop_weight.ptr<float>(y);
                const cv::Vec3b* f_ptr = crop_frame.ptr<cv::Vec3b>(y);
                cv::Vec3f* accum_ptr = accum_canvas.ptr<cv::Vec3f>(r1 + y);
                cv::Vec3f* weight_ptr = weight_canvas.ptr<cv::Vec3f>(r1 + y);

                for (int x = 0; x < c2 - c1; ++x) {
                    float w_val = w_ptr[x];
                    cv::Vec3b pix = f_ptr[x];
                    int cx = c1 + x;
                    accum_ptr[cx][0] += pix[0] * w_val;
                    accum_ptr[cx][1] += pix[1] * w_val;
                    accum_ptr[cx][2] += pix[2] * w_val;
                    weight_ptr[cx][0] += w_val;
                    weight_ptr[cx][1] += w_val;
                    weight_ptr[cx][2] += w_val;
                }
            }
        } else {
            for (int y = 0; y < r2 - r1; ++y) {
                const float* w_ptr = crop_weight.ptr<float>(y);
                const uchar* f_ptr = crop_frame.ptr<uchar>(y);
                float* accum_ptr = accum_canvas.ptr<float>(r1 + y);
                float* weight_ptr = weight_canvas.ptr<float>(r1 + y);

                for (int x = 0; x < c2 - c1; ++x) {
                    float w_val = w_ptr[x];
                    int cx = c1 + x;
                    accum_ptr[cx] += f_ptr[x] * w_val;
                    weight_ptr[cx] += w_val;
                }
            }
        }
    }

    cv::Mat result(canvas_height, canvas_width, target_type);
    if (channels == 3) {
        for (int y = 0; y < canvas_height; ++y) {
            const cv::Vec3f* accum_ptr = accum_canvas.ptr<cv::Vec3f>(y);
            const cv::Vec3f* weight_ptr = weight_canvas.ptr<cv::Vec3f>(y);
            cv::Vec3b* res_ptr = result.ptr<cv::Vec3b>(y);

            for (int x = 0; x < canvas_width; ++x) {
                cv::Vec3f w_val = weight_ptr[x];
                if (w_val[0] > 0.0f) {
                    res_ptr[x][0] = cv::saturate_cast<uchar>(std::round(accum_ptr[x][0] / w_val[0]));
                    res_ptr[x][1] = cv::saturate_cast<uchar>(std::round(accum_ptr[x][1] / w_val[1]));
                    res_ptr[x][2] = cv::saturate_cast<uchar>(std::round(accum_ptr[x][2] / w_val[2]));
                } else {
                    res_ptr[x] = cv::Vec3b(0, 0, 0);
                }
            }
        }
    } else {
        for (int y = 0; y < canvas_height; ++y) {
            const float* accum_ptr = accum_canvas.ptr<float>(y);
            const float* weight_ptr = weight_canvas.ptr<float>(y);
            uchar* res_ptr = result.ptr<uchar>(y);

            for (int x = 0; x < canvas_width; ++x) {
                float w_val = weight_ptr[x];
                if (w_val > 0.0f) {
                    res_ptr[x] = cv::saturate_cast<uchar>(std::round(accum_ptr[x] / w_val));
                } else {
                    res_ptr[x] = 0;
                }
            }
        }
    }

    return result;
}

} // namespace microstitch
