#include "MyStitcher.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

//#pragma once
//#include <afxwin.h>
//
//class CMyApp : public CWinApp
//{
//public:
//	virtual BOOL InitInstance();
//};
//
//extern CMyApp theApp;
//
//CMyApp theApp;
//
//BOOL CMyApp::InitInstance()
//{
//	CWinApp::InitInstance();
//
//	
//
//	return FALSE;
//}
int WinMain()
{
	return 0;
}

int main(int argc, char** argv)
{
    int cameraIndex = 0;
    if (argc > 1)
    {
        cameraIndex = std::atoi(argv[1]);
    }

    std::cout << "=========================================================" << std::endl;
    std::cout << "  MicroscopeStitcher - Live C++ Stitching Standalone Demo  " << std::endl;
    std::cout << "=========================================================" << std::endl;
    std::cout << "[INFO] Opening USB Microscope Camera index " << cameraIndex << "..." << std::endl;

    cv::VideoCapture cap;
#ifdef _WIN32
    // DirectShow is recommended for Windows USB microscopes
    cap.open(cameraIndex, cv::CAP_DSHOW);
#endif
    if (!cap.isOpened())
    {
        cap.open(cameraIndex);
    }

    if (!cap.isOpened())
    {
        std::cerr << "[ERROR] Could not open camera index " << cameraIndex << "." << std::endl;
        std::cerr << "Usage: MicroscopeStitcher.exe [camera_index]" << std::endl;
        return 1;
    }

    // Configure camera frame size
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 360);

    const std::string kWinFrame  = "Current Frame";
    const std::string kWinMosaic = "Live Mosaic";

    cv::namedWindow(kWinFrame, cv::WINDOW_AUTOSIZE);
    cv::namedWindow(kWinMosaic, cv::WINDOW_AUTOSIZE);

    // Instantiate the existing C++ Stitcher engine
    microstitch::Stitcher stitcher;

    std::cout << "[INFO] Camera opened successfully." << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  ESC / Q - Quit application" << std::endl;
    std::cout << "  R       - Reset stitched mosaic" << std::endl;
    std::cout << "  S       - Save current mosaic image" << std::endl;

    cv::Mat frame;
    int frameCount = 0;

    while (true)
    {
        if (!cap.read(frame) || frame.empty())
        {
            std::cerr << "[WARNING] Empty frame received from camera." << std::endl;
            cv::waitKey(10);
            continue;
        }

        frameCount++;

        // Show live camera frame
        cv::imshow(kWinFrame, frame);

        // Process frame with existing C++ stitching engine
        bool accepted = stitcher.processFrame(frame);

        // Display updated mosaic if available
        if (stitcher.hasMosaic())
        {
            cv::Mat mosaic = stitcher.getMosaic();
            if (!mosaic.empty())
            {
                cv::imshow(kWinMosaic, mosaic);
            }
        }

        const int key = cv::waitKey(1) & 0xFF;
        if (key == 27 || key == 'q' || key == 'Q')
        {
            std::cout << "[INFO] Exiting..." << std::endl;
            break;
        }
        else if (key == 'r' || key == 'R')
        {
            stitcher.reset();
            std::cout << "[INFO] Mosaic reset at frame " << frameCount << std::endl;
        }
        else if (key == 's' || key == 'S')
        {
            if (stitcher.hasMosaic())
            {
                cv::Mat mosaic = stitcher.getMosaic();
                if (!mosaic.empty())
                {
                    std::string filename = "live_mosaic_" + std::to_string(frameCount) + ".png";
                    cv::imwrite(filename, mosaic);
                    std::cout << "[INFO] Saved mosaic to " << filename << std::endl;
                }
            }
            else
            {
                std::cout << "[WARNING] No mosaic available to save yet." << std::endl;
            }
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
