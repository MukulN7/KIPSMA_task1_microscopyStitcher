
// KStitcherDlg.h : header file
//

#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/stitching.hpp>
#include "MyStitcher.h"

using namespace cv;
// CKStitcherDlg dialog
class CKStitcherDlg : public CDialogEx
{
// Construction
public:
	CKStitcherDlg(CWnd* pParent = NULL);	// standard constructor
	BOOL SetRegistryKeyStitch(HKEY hkey, LPCTSTR subKey, LPCTSTR lpszRegistryKey);
	BOOL GetRegistryKeyStitch(HKEY hkey, LPCTSTR subKey, LPCTSTR valueName, CString& registryValue);
	BOOL GetRegistryKeyStitch(HKEY hkey, LPCTSTR subKey, LPCTSTR valueName, int& registryValue);

	Mat DrawPicToHDC(cv::Mat  cvImg, UINT nDlgID, bool bMaintainAspectRatio /* =true*/);
	Mat DrawPicToHDC1(cv::Mat  cvImg, UINT nDlgID, bool bMaintainAspectRatio /* =true*/);
	void DrawMatToHDC_CVV(Mat img, UINT ID);
	void DrawMatToHDC(Mat img, UINT ID);
	void AdjustAspectImageSize(const cv::Size& imageSize, const cv::Size& destSize, cv::Size& newSize);
	
	BOOL bChange;
	cv::VideoCapture		capture;
	CString sCamName; int nCamIndex;
	Mat frame;
	int  offsetX;
	int  offsetY;
	BOOL bCapture;

	// Instantiate the existing C++ Stitcher engine
	microstitch::Stitcher stitcher;

	//BOOL bChange;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_KSTITCHER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnCapture();
};
