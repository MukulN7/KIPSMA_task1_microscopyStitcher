#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

namespace microstitch {

//struct RegistrationResult {
//    double dx = 0.0;
//    double dy = 0.0;
//    double response = 0.0;
//    bool valid = false;
//    double spatial_score = 0.0;
//    double quality_score = 0.0;
//};

	struct RegistrationResult {
		double dx;
		double dy;
		double response;
		bool valid;
		double spatial_score;
		double quality_score;

		RegistrationResult()
			: dx(0.0),
			dy(0.0),
			response(0.0),
			valid(false),
			spatial_score(0.0),
			quality_score(0.0) {}

		RegistrationResult(
			double dx_,
			double dy_,
			double response_,
			bool valid_,
			double spatial_score_,
			double quality_score_)
			: dx(dx_),
			dy(dy_),
			response(response_),
			valid(valid_),
			spatial_score(spatial_score_),
			quality_score(quality_score_) {}
	};

class RegistrationEngine {
public:
    RegistrationEngine(
        double min_response = 0.05,
        double min_spatial_score = 0.50,
        double max_shift = 150.0,
        bool use_laplacian = true,
        bool unwrap_aliasing = true,
        double min_fallback_response = -1.0,
        bool use_sift = true
    );

    RegistrationResult registerFrames(const cv::Mat& ref_frame, const cv::Mat& curr_frame);

    static double computeSpatialOverlapScore(
        const cv::Mat& img1,
        const cv::Mat& img2,
        double shift_x,
        double shift_y
    );

private:
    RegistrationResult registerSift(const cv::Mat& ref_frame, const cv::Mat& curr_frame);
    RegistrationResult registerPhaseCorrelation(const cv::Mat& ref_frame, const cv::Mat& curr_frame);
    RegistrationResult correlateFullFrame(const cv::Mat& ref_prep, const cv::Mat& curr_prep);
    RegistrationResult tryHorizontalOverlap(const cv::Mat& ref_frame, const cv::Mat& curr_frame);
    RegistrationResult correlateStrip(
        const cv::Mat& ref_strip,
        const cv::Mat& curr_strip,
        double global_dx_offset,
        double global_dy_offset
    );
    void unwrapShiftWithScore(
        const cv::Mat& img1,
        const cv::Mat& img2,
        double raw_dx,
        double raw_dy,
        double& best_dx,
        double& best_dy,
        double& best_score
    );

    cv::Mat preprocess(const cv::Mat& frame);
    cv::Mat toGrayU8(const cv::Mat& frame);

    double min_response_;
    double min_spatial_score_;
    double max_shift_;
    bool use_laplacian_;
    bool unwrap_aliasing_;
    double min_fallback_response_;
    bool use_sift_;
};

} // namespace microstitch
