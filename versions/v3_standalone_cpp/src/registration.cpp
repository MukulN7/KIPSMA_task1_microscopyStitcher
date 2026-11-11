#include "registration.h"
#include <algorithm>
#include <limits>
#include <set>

namespace microstitch {

static const double _HORIZONTAL_OVERLAP_FRACTIONS[] = { 0.05, 0.10, 0.15, 0.20, 0.30, 0.40 };

RegistrationEngine::RegistrationEngine(
    double min_response,
    double min_spatial_score,
    double max_shift,
    bool use_laplacian,
    bool unwrap_aliasing,
    double min_fallback_response,
    bool use_sift
) : min_response_(min_response),
    min_spatial_score_(min_spatial_score),
    max_shift_(max_shift),
    use_laplacian_(use_laplacian),
    unwrap_aliasing_(unwrap_aliasing),
    min_fallback_response_(min_fallback_response < 0 ? min_response : min_fallback_response),
    use_sift_(use_sift)
{
}

RegistrationResult RegistrationEngine::registerFrames(const cv::Mat& ref_frame, const cv::Mat& curr_frame) {
    if (ref_frame.empty() || curr_frame.empty()) {
        return RegistrationResult();
    }
    if (ref_frame.size() != curr_frame.size()) {
        return RegistrationResult();
    }

    // 1. Primary path: SIFT feature matching + RANSAC translation
    if (use_sift_) {
        RegistrationResult sift_result = registerSift(ref_frame, curr_frame);
        if (sift_result.valid) {
            return sift_result;
        }
    }

    // 2. Fallback path: Phase correlation (full-frame -> strip fallback)
    return registerPhaseCorrelation(ref_frame, curr_frame);
}

RegistrationResult RegistrationEngine::registerSift(const cv::Mat& ref_frame, const cv::Mat& curr_frame) {
    try {
        cv::Mat gray_ref = toGrayU8(ref_frame);
        cv::Mat gray_curr = toGrayU8(curr_frame);

        cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
        std::vector<cv::KeyPoint> kp_ref, kp_curr;
        cv::Mat des_ref, des_curr;

        sift->detectAndCompute(gray_ref, cv::noArray(), kp_ref, des_ref);
        sift->detectAndCompute(gray_curr, cv::noArray(), kp_curr, des_curr);

        if (des_ref.empty() || des_curr.empty() || kp_ref.size() < 6 || kp_curr.size() < 6) {
            return RegistrationResult();
        }

        cv::BFMatcher matcher(cv::NORM_L2);
        std::vector<std::vector<cv::DMatch>> matches;
        matcher.knnMatch(des_ref, des_curr, matches, 2);

        std::vector<cv::DMatch> good_matches;
        std::vector<cv::Point2f> src_pts, dst_pts;

        for (size_t i = 0; i < matches.size(); ++i) {
            if (matches[i].size() == 2) {
                const auto& m = matches[i][0];
                const auto& n = matches[i][1];
                if (m.distance < 0.75f * n.distance) {
                    good_matches.push_back(m);
                    src_pts.push_back(kp_ref[m.queryIdx].pt);
                    dst_pts.push_back(kp_curr[m.trainIdx].pt);
                }
            }
        }

        if (good_matches.size() < 6) {
            return RegistrationResult();
        }

        cv::Mat inliers;
        cv::Mat matrix = cv::estimateAffinePartial2D(src_pts, dst_pts, inliers, cv::RANSAC, 3.0);

        if (matrix.empty()) {
            return RegistrationResult();
        }

        double a = matrix.at<double>(0, 0);
        double b = matrix.at<double>(1, 0);
        double scale = std::hypot(a, b);
        double angle_deg = std::abs(std::atan2(b, a) * 180.0 / CV_PI);
        double dx = matrix.at<double>(0, 2);
        double dy = matrix.at<double>(1, 2);
        double shift_dist = std::hypot(dx, dy);

        int inlier_count = 0;
        if (!inliers.empty()) {
            inlier_count = cv::countNonZero(inliers);
        }
        double inlier_ratio = static_cast<double>(inlier_count) / static_cast<double>(good_matches.size());

        cv::Mat r_gray, c_gray;
        gray_ref.convertTo(r_gray, CV_32F);
        gray_curr.convertTo(c_gray, CV_32F);

        double spatial_score = computeSpatialOverlapScore(r_gray, c_gray, dx, dy);
        double quality_score = std::max(0.0, std::min(1.0, spatial_score));

        bool valid = (std::abs(scale - 1.0) <= 0.05) &&
                     (angle_deg <= 3.0) &&
                     (inlier_count >= 5) &&
                     (inlier_ratio >= 0.45) &&
                     (inlier_count >= 10 || inlier_ratio >= 0.50) &&
                     (max_shift_ <= 0.0 || shift_dist <= max_shift_) &&
                     std::isfinite(dx) && std::isfinite(dy);

        return RegistrationResult{ dx, dy, inlier_ratio, valid, spatial_score, quality_score };
    } catch (...) {
        return RegistrationResult();
    }
}

cv::Mat RegistrationEngine::toGrayU8(const cv::Mat& frame) {
    cv::Mat gray;
    if (frame.channels() == 3) {
        if (frame.depth() == CV_8U) {
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        } else {
            cv::Mat temp;
            cv::cvtColor(frame, temp, cv::COLOR_BGR2GRAY);
            double mn, mx;
            cv::minMaxLoc(temp, &mn, &mx);
            double rng = (mx != mn) ? (mx - mn) : 1.0;
            temp.convertTo(gray, CV_8U, 255.0 / rng, -mn * 255.0 / rng);
        }
    } else {
        if (frame.depth() == CV_8U) {
            gray = frame.clone();
        } else {
            double mn, mx;
            cv::minMaxLoc(frame, &mn, &mx);
            double rng = (mx != mn) ? (mx - mn) : 1.0;
            frame.convertTo(gray, CV_8U, 255.0 / rng, -mn * 255.0 / rng);
        }
    }
    return gray;
}

RegistrationResult RegistrationEngine::registerPhaseCorrelation(const cv::Mat& ref_frame, const cv::Mat& curr_frame) {
    cv::Mat ref_prep = preprocess(ref_frame);
    cv::Mat curr_prep = preprocess(curr_frame);

    RegistrationResult result = correlateFullFrame(ref_prep, curr_prep);

    if (!result.valid) {
        RegistrationResult fallback = tryHorizontalOverlap(ref_frame, curr_frame);
        if (fallback.valid) {
            return fallback;
        }
        return result;
    }

    return result;
}

RegistrationResult RegistrationEngine::correlateFullFrame(const cv::Mat& ref_prep, const cv::Mat& curr_prep) {
    int h = ref_prep.rows;
    int w = ref_prep.cols;

    cv::Mat hann;
    cv::createHanningWindow(hann, cv::Size(w, h), CV_64F);

    cv::Mat ref_64, curr_64;
    ref_prep.convertTo(ref_64, CV_64F);
    curr_prep.convertTo(curr_64, CV_64F);

    double response = 0.0;
    cv::Point2d shift = cv::phaseCorrelate(ref_64, curr_64, hann, &response);

    if (!std::isfinite(shift.x) || !std::isfinite(shift.y) || !std::isfinite(response)) {
        return RegistrationResult();
    }

    double dx = shift.x;
    double dy = shift.y;
    double spatial_score = 0.0;

    if (unwrap_aliasing_) {
        unwrapShiftWithScore(ref_prep, curr_prep, shift.x, shift.y, dx, dy, spatial_score);
    } else {
        spatial_score = computeSpatialOverlapScore(ref_prep, curr_prep, dx, dy);
    }

    double quality_score = std::max(0.0, std::min(1.0, spatial_score));
    double shift_dist = std::hypot(dx, dy);

    if (max_shift_ > 0.0 && shift_dist > max_shift_) {
        return RegistrationResult{ dx, dy, response, false, spatial_score, quality_score };
    }

    bool valid = (response >= min_response_) &&
                 (spatial_score >= min_spatial_score_) &&
                 std::isfinite(dx) && std::isfinite(dy);

    return RegistrationResult{ dx, dy, response, valid, spatial_score, quality_score };
}

RegistrationResult RegistrationEngine::tryHorizontalOverlap(const cv::Mat& ref_frame, const cv::Mat& curr_frame) {
    int h = ref_frame.rows;
    int w = ref_frame.cols;
    if (w < 4) {
        return RegistrationResult();
    }

    RegistrationResult best_result;
    double best_score = -1.0;

    for (double frac : _HORIZONTAL_OVERLAP_FRACTIONS) {
        int strip_w = std::max(2, static_cast<int>(std::round(frac * w)));
        if (strip_w >= w) continue;

        // Right hypothesis
        cv::Mat ref_strip = ref_frame(cv::Rect(w - strip_w, 0, strip_w, h));
        cv::Mat curr_strip = curr_frame(cv::Rect(0, 0, strip_w, h));

        RegistrationResult r_result = correlateStrip(ref_strip, curr_strip, static_cast<double>(w - strip_w), 0.0);
        if (r_result.valid && r_result.spatial_score > best_score) {
            best_score = r_result.spatial_score;
            best_result = r_result;
        }

        // Left hypothesis
        ref_strip = ref_frame(cv::Rect(0, 0, strip_w, h));
        curr_strip = curr_frame(cv::Rect(w - strip_w, 0, strip_w, h));

        RegistrationResult l_result = correlateStrip(ref_strip, curr_strip, -static_cast<double>(w - strip_w), 0.0);
        if (l_result.valid && l_result.spatial_score > best_score) {
            best_score = l_result.spatial_score;
            best_result = l_result;
        }
    }

    return best_result;
}

RegistrationResult RegistrationEngine::correlateStrip(
    const cv::Mat& ref_strip,
    const cv::Mat& curr_strip,
    double global_dx_offset,
    double global_dy_offset
) {
    int sh = ref_strip.rows;
    int sw = ref_strip.cols;
    if (sh < 2 || sw < 2) return RegistrationResult();

    cv::Mat ref_prep = preprocess(ref_strip);
    cv::Mat curr_prep = preprocess(curr_strip);

    cv::Mat hann;
    cv::createHanningWindow(hann, cv::Size(sw, sh), CV_64F);

    cv::Mat ref_64, curr_64;
    ref_prep.convertTo(ref_64, CV_64F);
    curr_prep.convertTo(curr_64, CV_64F);

    double response = 0.0;
    cv::Point2d local_shift;
    try {
        local_shift = cv::phaseCorrelate(ref_64, curr_64, hann, &response);
    } catch (...) {
        return RegistrationResult();
    }

    if (!std::isfinite(local_shift.x) || !std::isfinite(local_shift.y) || !std::isfinite(response)) {
        return RegistrationResult();
    }

    double dx = global_dx_offset + local_shift.x;
    double dy = global_dy_offset + local_shift.y;

    cv::Mat r_gray = toGrayU8(ref_strip);
    cv::Mat c_gray = toGrayU8(curr_strip);
    cv::Mat r_float, c_float;
    r_gray.convertTo(r_float, CV_32F);
    c_gray.convertTo(c_float, CV_32F);

    double strip_spatial = computeSpatialOverlapScore(r_float, c_float, local_shift.x, local_shift.y);
    double quality_score = std::max(0.0, std::min(1.0, strip_spatial));

    if (max_shift_ > 0.0 && std::hypot(dx, dy) > max_shift_) {
        return RegistrationResult{ dx, dy, response, false, strip_spatial, quality_score };
    }

    bool valid = (response >= min_fallback_response_) &&
                 (strip_spatial >= min_spatial_score_) &&
                 std::isfinite(dx) && std::isfinite(dy);

    return RegistrationResult{ dx, dy, response, valid, strip_spatial, quality_score };
}

void RegistrationEngine::unwrapShiftWithScore(
    const cv::Mat& img1,
    const cv::Mat& img2,
    double raw_dx,
    double raw_dy,
    double& best_dx,
    double& best_dy,
    double& best_score
) {
    int h = img1.rows;
    int w = img1.cols;

    std::set<double> x_cand{ raw_dx };
    if (raw_dx > 0) x_cand.insert(raw_dx - w);
    else if (raw_dx < 0) x_cand.insert(raw_dx + w);

    std::set<double> y_cand{ raw_dy };
    if (raw_dy > 0) y_cand.insert(raw_dy - h);
    else if (raw_dy < 0) y_cand.insert(raw_dy + h);

    best_dx = raw_dx;
    best_dy = raw_dy;
    best_score = -1.0;

    for (double cx : x_cand) {
        for (double cy : y_cand) {
            double score = computeSpatialOverlapScore(img1, img2, cx, cy);
            if (score > best_score) {
                best_score = score;
                best_dx = cx;
                best_dy = cy;
            }
        }
    }
}

double RegistrationEngine::computeSpatialOverlapScore(
    const cv::Mat& img1,
    const cv::Mat& img2,
    double shift_x,
    double shift_y
) {
    int h = img1.rows;
    int w = img1.cols;

    int sx = static_cast<int>(std::round(shift_x));
    int sy = static_cast<int>(std::round(shift_y));

    int x1_min = std::max(0, -sx);
    int x1_max = std::min(w, w - sx);
    int y1_min = std::max(0, -sy);
    int y1_max = std::min(h, h - sy);

    if (x1_max <= x1_min || y1_max <= y1_min) {
        return -1.0;
    }

    int x2_min = x1_min + sx;
    int x2_max = x1_max + sx;
    int y2_min = y1_min + sy;
    int y2_max = y1_max + sy;

    cv::Mat overlap1 = img1(cv::Rect(x1_min, y1_min, x1_max - x1_min, y1_max - y1_min));
    cv::Mat overlap2 = img2(cv::Rect(x2_min, y2_min, x2_max - x2_min, y2_max - y2_min));

    double norm1 = cv::norm(overlap1);
    double norm2 = cv::norm(overlap2);

    if (norm1 == 0.0 || norm2 == 0.0) {
        return -1.0;
    }

    double prod = overlap1.dot(overlap2);
    return prod / (norm1 * norm2);
}

cv::Mat RegistrationEngine::preprocess(const cv::Mat& frame) {
    cv::Mat gray;
    if (frame.channels() == 3) {
        cv::Mat temp = toGrayU8(frame);
        temp.convertTo(gray, CV_32F);
    } else {
        frame.convertTo(gray, CV_32F);
    }

    if (!use_laplacian_) {
        return gray;
    }

    double min_val, max_val;
    cv::minMaxLoc(gray, &min_val, &max_val);

    cv::Mat norm;
    if (max_val > min_val) {
        norm = (gray - min_val) / (max_val - min_val);
    } else {
        norm = cv::Mat::zeros(gray.size(), CV_32F);
    }

    cv::Mat lap;
    cv::Laplacian(norm, lap, CV_32F, 3);
    return lap;
}

} // namespace microstitch
