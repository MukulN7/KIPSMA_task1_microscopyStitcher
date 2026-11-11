cmake_minimum_required(VERSION 3.16)
project(MicroscopeStitcher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to C:/tools/opencv3411 if OpenCV_DIR is not specified
if(NOT DEFINED OpenCV_DIR AND EXISTS "C:/tools/opencv3411")
    set(OpenCV_DIR "C:/tools/opencv3411" CACHE PATH "Path to OpenCV build directory")
endif()

find_package(OpenCV QUIET COMPONENTS core imgproc imgcodecs highgui videoio)

if(NOT OpenCV_FOUND)
    # Robust fallback search for OpenCV 3.4.11 or custom OpenCV installations
    find_path(OpenCV_INCLUDE_DIRS "opencv2/opencv.hpp"
        PATHS "${OpenCV_DIR}/include" "${OpenCV_DIR}/../include" "C:/tools/opencv3411/include"
        NO_DEFAULT_PATH)
    if(NOT OpenCV_INCLUDE_DIRS)
        find_path(OpenCV_INCLUDE_DIRS "opencv2/opencv.hpp")
    endif()

    find_library(OpenCV_CORE_LIB NAMES opencv_core3411 opencv_core PATHS "${OpenCV_DIR}/lib" "C:/tools/opencv3411/lib" NO_DEFAULT_PATH)
    find_library(OpenCV_IMGPROC_LIB NAMES opencv_imgproc3411 opencv_imgproc PATHS "${OpenCV_DIR}/lib" "C:/tools/opencv3411/lib" NO_DEFAULT_PATH)
    find_library(OpenCV_FEATURES2D_LIB NAMES opencv_features2d3411 opencv_features2d PATHS "${OpenCV_DIR}/lib" "C:/tools/opencv3411/lib" NO_DEFAULT_PATH)
    find_library(OpenCV_CALIB3D_LIB NAMES opencv_calib3d3411 opencv_calib3d PATHS "${OpenCV_DIR}/lib" "C:/tools/opencv3411/lib" NO_DEFAULT_PATH)
    find_library(OpenCV_FLANN_LIB NAMES opencv_flann3411 opencv_flann PATHS "${OpenCV_DIR}/lib" "C:/tools/opencv3411/lib" NO_DEFAULT_PATH)
    find_library(OpenCV_IMGCODECS_LIB NAMES opencv_imgcodecs3411 opencv_imgcodecs PATHS "${OpenCV_DIR}/lib" "C:/tools/opencv3411/lib" NO_DEFAULT_PATH)
    find_library(OpenCV_HIGHGUI_LIB NAMES opencv_highgui3411 opencv_highgui PATHS "${OpenCV_DIR}/lib" "C:/tools/opencv3411/lib" NO_DEFAULT_PATH)
    find_library(OpenCV_VIDEOIO_LIB NAMES opencv_videoio3411 opencv_videoio PATHS "${OpenCV_DIR}/lib" "C:/tools/opencv3411/lib" NO_DEFAULT_PATH)

    if(OpenCV_CORE_LIB AND OpenCV_IMGPROC_LIB AND OpenCV_INCLUDE_DIRS)
        set(OpenCV_LIBS ${OpenCV_CORE_LIB} ${OpenCV_IMGPROC_LIB} ${OpenCV_FEATURES2D_LIB} ${OpenCV_CALIB3D_LIB} ${OpenCV_FLANN_LIB} ${OpenCV_IMGCODECS_LIB} ${OpenCV_HIGHGUI_LIB} ${OpenCV_VIDEOIO_LIB})
        set(OpenCV_FOUND TRUE)
        message(STATUS "Found OpenCV via fallback: ${OpenCV_LIBS}")
    else()
        message(FATAL_ERROR "Could NOT find OpenCV 3.4.11 installation.")
    endif()
endif()

# Set output binary directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

# Export macro to compile header cleanly directly in executable
add_definitions(-DMICROSTITCH_EXPORTS)

set(STITCHER_SOURCES
    src/main.cpp
    src/registration.cpp
    src/mosaic.cpp
    src/mosaic_renderer.cpp
    src/stitcher.cpp
)

set(STITCHER_HEADERS
    include/microstitch/stitcher.h
    src/registration.h
    src/mosaic.h
    src/mosaic_renderer.h
    src/stitcher.h
)

# Build Standalone Executable (no DLL, no Python, no MFC)
add_executable(MicroscopeStitcher ${STITCHER_SOURCES} ${STITCHER_HEADERS})

target_include_directories(MicroscopeStitcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(MicroscopeStitcher PRIVATE ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(MicroscopeStitcher PRIVATE /W4 /utf-8)
endif()

