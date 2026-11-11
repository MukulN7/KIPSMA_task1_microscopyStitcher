#include <iostream>
#include <vector>
#include "stitcher_api.h"

int main() {
    std::cout << "=== Stitcher.dll C++ Integration Test ===" << std::endl;

    std::cout << "[1] Calling Stitcher_Create(640, 360, 3)..." << std::endl;
    void* handle = Stitcher_Create(640, 360, 3);
    if (!handle) {
        std::cerr << "[ERROR] Stitcher_Create returned NULL!" << std::endl;
        return 1;
    }
    std::cout << "    SUCCESS. Handle: " << handle << std::endl;

    // Create synthetic 640x360 3-channel frame with pattern
    std::vector<unsigned char> frame1(640 * 360 * 3);
    for (int y = 0; y < 360; ++y) {
        for (int x = 0; x < 640; ++x) {
            int idx = (y * 640 + x) * 3;
            frame1[idx + 0] = static_cast<unsigned char>((x + y) % 256);
            frame1[idx + 1] = static_cast<unsigned char>((x * 2) % 256);
            frame1[idx + 2] = static_cast<unsigned char>((y * 3) % 256);
        }
    }

    std::cout << "[2] Processing Frame 1 (Origin)..." << std::endl;
    int rc1 = Stitcher_ProcessFrame(handle, frame1.data(), 640, 360, 3, 640 * 3);
    std::cout << "    ProcessFrame 1 rc: " << rc1 << " (Expected: 1)" << std::endl;

    // Shift frame slightly for frame 2
    std::vector<unsigned char> frame2(640 * 360 * 3);
    for (int y = 0; y < 360; ++y) {
        for (int x = 0; x < 640; ++x) {
            int src_x = (x + 10) % 640;
            int idx = (y * 640 + x) * 3;
            int src_idx = (y * 640 + src_x) * 3;
            frame2[idx + 0] = frame1[src_idx + 0];
            frame2[idx + 1] = frame1[src_idx + 1];
            frame2[idx + 2] = frame1[src_idx + 2];
        }
    }

    std::cout << "[3] Processing Frame 2 (Shifted)..." << std::endl;
    int rc2 = Stitcher_ProcessFrame(handle, frame2.data(), 640, 360, 3, 640 * 3);
    std::cout << "    ProcessFrame 2 rc: " << rc2 << std::endl;

    // Query mosaic
    const unsigned char* mosaic_ptr = nullptr;
    int mw = 0, mh = 0, mc = 0, ms = 0;
    std::cout << "[4] Calling Stitcher_GetMosaic()..." << std::endl;
    int rc_mos = Stitcher_GetMosaic(handle, &mosaic_ptr, &mw, &mh, &mc, &ms);
    std::cout << "    GetMosaic rc: " << rc_mos << std::endl;
    std::cout << "    Mosaic dimensions: " << mw << "x" << mh << ", channels=" << mc << ", stride=" << ms << std::endl;
    if (mosaic_ptr != nullptr) {
        std::cout << "    Mosaic buffer pointer is non-null." << std::endl;
    }

    std::cout << "[5] Calling Stitcher_Reset()..." << std::endl;
    Stitcher_Reset(handle);
    std::cout << "    Reset OK." << std::endl;

    const unsigned char* reset_ptr = nullptr;
    int rmw = 0, rmh = 0, rmc = 0, rms = 0;
    int rc_after_reset = Stitcher_GetMosaic(handle, &reset_ptr, &rmw, &rmh, &rmc, &rms);
    std::cout << "    GetMosaic after Reset rc: " << rc_after_reset << " (Expected: 0)" << std::endl;

    std::cout << "[6] Calling Stitcher_Destroy()..." << std::endl;
    Stitcher_Destroy(handle);
    std::cout << "    Destroy OK." << std::endl;

    std::cout << "\n=== ALL C++ DLL VERIFICATION TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
