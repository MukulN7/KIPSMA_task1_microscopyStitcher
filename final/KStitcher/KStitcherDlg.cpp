
// KStitcherDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KStitcher.h"
#include "KStitcherDlg.h"
#include "afxdialogex.h"
#include "CvvImage.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CAMERA  WM_USER + 1107

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CKStitcherDlg dialog



CKStitcherDlg::CKStitcherDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_KSTITCHER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKStitcherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKStitcherDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_CAPTURE, &CKStitcherDlg::OnBnClickedBtnCapture)
END_MESSAGE_MAP()


// CKStitcherDlg message handlers

BOOL CKStitcherDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	ShowWindow(SW_MAXIMIZE);
	
	CString subKey =
		_T("Software\\K-Industry Pro\\Industry Pro\\Settings");

	//CString camName;

	if (GetRegistryKeyStitch(
		HKEY_CURRENT_USER,
		subKey,
		_T("Default Cam Name"),
		sCamName))
	{
		//AfxMessageBox(sCamName);

		// Example: assign it to your camera name variable
		// m_strCameraName = camName;
	}
	else
	{
		AfxMessageBox(_T("Failed to read Default Cam Name."));
	}
	

	if (GetRegistryKeyStitch(
		HKEY_CURRENT_USER,
		subKey,
		_T("Default Cam"),
		nCamIndex))
	{
		CString msg;
		msg.Format(_T("Default Cam = %d"), nCamIndex);
		//AfxMessageBox(msg);
	}
	else
	{
		AfxMessageBox(_T("Failed to read Default Cam Name."));
	}
	//CWinApp* pApp = AfxGetApp();
	//CString sCamName = pApp->GetProfileStringW(_T("Settings"), _T("Default Cam Name"), L"");
	//int nCamIndex = pApp->GetProfileIntW(_T("Settings"), _T("Default Cam"), 0);
	
	if (capture.open(nCamIndex, CAP_DSHOW) == false)
	{
		if (capture.open(nCamIndex, CAP_ANY) == false)
		{
			MessageBox(L"Unable to start camera", CString((LPCSTR)IDS_TITLE), MB_OK | MB_ICONWARNING);
			return FALSE;
		}
	}
	//bCamSuccess = TRUE;
	int Width;// = pApp->GetProfileInt(_T("Settings"), _T("Cam-wResoluion"), 0);
	int Height;// = pApp->GetProfileInt(_T("Settings"), _T("Cam-hResoluion"), 0);
	

	//capture.set(CV_CAP_PROP_FRAME_WIDTH, Width);
	//capture.set(CV_CAP_PROP_FRAME_HEIGHT, Height);
	SetTimer(CAMERA, 50, 0);
	bChange = TRUE;

	// Configure camera frame size
	capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
	capture.set(cv::CAP_PROP_FRAME_HEIGHT, 360);

	//const std::string kWinFrame = "Current Frame";
	//const std::string kWinMosaic = "Live Mosaic";

	//cv::namedWindow(kWinFrame, cv::WINDOW_AUTOSIZE);
	

	


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CKStitcherDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CKStitcherDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CKStitcherDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CKStitcherDlg::SetRegistryKeyStitch(HKEY hkey, LPCTSTR subKey, LPCTSTR lpszRegistryKey)
{	
	if (subKey == NULL || lpszRegistryKey == NULL)
		return FALSE;

	HKEY hSubKey = NULL;

	// Create the registry key if it does not exist.
	LONG result = RegCreateKeyEx(
		hkey,
		subKey,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_SET_VALUE,
		NULL,
		&hSubKey,
		NULL
	);

	if (result != ERROR_SUCCESS)
		return FALSE;

	// Write the default value (unnamed value) as REG_SZ.
	DWORD dataSize =
		(DWORD)((_tcslen(lpszRegistryKey) + 1) * sizeof(TCHAR));

	result = RegSetValueEx(
		hSubKey,
		NULL,                       // Default value
		0,
		REG_SZ,
		(const BYTE*)lpszRegistryKey,
		dataSize
	);

	RegCloseKey(hSubKey);

	return (result == ERROR_SUCCESS) ? TRUE : FALSE;
	

}

BOOL CKStitcherDlg::GetRegistryKeyStitch(HKEY hkey, LPCTSTR subKey, LPCTSTR valueName, CString& registryValue)
{
	if (subKey == NULL || valueName == NULL)
		return FALSE;

	HKEY hSubKey = NULL;

	LONG result = RegOpenKeyEx(
		hkey,
		subKey,
		0,
		KEY_QUERY_VALUE,
		&hSubKey
	);

	if (result != ERROR_SUCCESS)
		return FALSE;

	DWORD dataType = 0;
	DWORD dataSize = 0;

	// Get the required buffer size
	result = RegQueryValueEx(
		hSubKey,
		valueName,
		NULL,
		&dataType,
		NULL,
		&dataSize
	);

	if (result != ERROR_SUCCESS || dataType != REG_SZ)
	{
		RegCloseKey(hSubKey);
		return FALSE;
	}

	// Allocate enough space, including the null terminator
	DWORD charCount = dataSize / sizeof(TCHAR);

	LPTSTR buffer = registryValue.GetBuffer(
		static_cast<int>(charCount)
	);

	result = RegQueryValueEx(
		hSubKey,
		valueName,
		NULL,
		&dataType,
		reinterpret_cast<LPBYTE>(buffer),
		&dataSize
	);

	RegCloseKey(hSubKey);

	registryValue.ReleaseBuffer();

	return (result == ERROR_SUCCESS && dataType == REG_SZ);
}

BOOL CKStitcherDlg::GetRegistryKeyStitch(HKEY hkey, LPCTSTR subKey, LPCTSTR valueName, int& registryValue)
{
	if (subKey == NULL || valueName == NULL)
		return FALSE;

	HKEY hSubKey = NULL;

	LONG result = RegOpenKeyEx(
		hkey,
		subKey,
		0,
		KEY_QUERY_VALUE,
		&hSubKey
	);

	if (result != ERROR_SUCCESS)
		return FALSE;

	DWORD dataType = 0;
	DWORD dataSize = sizeof(DWORD);
	DWORD data = 0;

	result = RegQueryValueEx(
		hSubKey,
		valueName,
		NULL,
		&dataType,
		reinterpret_cast<LPBYTE>(&data),
		&dataSize
	);

	RegCloseKey(hSubKey);

	if (result != ERROR_SUCCESS ||
		dataType != REG_DWORD ||
		dataSize != sizeof(DWORD))
	{
		return FALSE;
	}

	registryValue = static_cast<int>(data);

	return TRUE;
}

void CKStitcherDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == CAMERA)
	{
		//if (bPause == FALSE)
		{
			Mat frame1;
			//pCamInput->read(frame1);			
			{
				capture.read(frame1);
				//flip(frame, frame, 0);
				if (!frame1.empty())
				{
					// Process original full-resolution camera frame with stitching engine
					bool accepted = stitcher.processFrame(frame1);

					// Generate scaled preview copy exclusively for camera preview control
					frame = DrawPicToHDC(frame1, IDC_CAM_FRAME, true);
					DrawMatToHDC(frame, IDC_CAM_FRAME);

					// Display updated mosaic if available
					if (stitcher.hasMosaic())
					{
						cv::Mat mosaic = stitcher.getMosaic();
						if (!mosaic.empty())
						{
							//const std::string kWinMosaic = "Live Mosaic";
							//cv::namedWindow(kWinMosaic, cv::WINDOW_AUTOSIZE);
							DrawMatToHDC(mosaic, IDC_STITCHED_IMG);
							//cv::imshow(kWinMosaic, mosaic);
							if (bCapture == TRUE)
							{
								bCapture = FALSE;
								TCHAR szFilters[] = _T("Images (*.jpg)|*.jpg|All Files (*.*)|*.*||");

								// Create an Open dialog; the default file name extension is ".my".
								CFileDialog fileDlg(FALSE, _T("jpg"), _T("*.jpg"),
									OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);

								// Display the file dialog. When user clicks OK, fileDlg.DoModal() 
								// returns IDOK.
								if (fileDlg.DoModal() == IDOK)
								{
									CString pathName = fileDlg.GetPathName();
									CT2CA mypath(pathName);
									std::string path = (mypath);

									//Mat dst;
									//cv::resize(frame, dst, cv::Size(nImgWidth, nImgHeight));
									cv::imwrite(path, mosaic);
								}
							}
						}
					}
				}
			}
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

Mat CKStitcherDlg::DrawPicToHDC(cv::Mat  cvImg, UINT nDlgID, bool bMaintainAspectRatio /* =true*/)
{
	// Get the HDC handle information from the ID passed
	CDC* pDC = GetDlgItem(nDlgID)->GetDC();
	HDC  hDC = pDC->GetSafeHdc();

	CRect rect;
	GetDlgItem(nDlgID)->GetClientRect(rect);

	cv::Size winSize(rect.right, rect.bottom);

	// Calculate the size of the image that
	// will fit in the control rectangle.
	cv::Size origImageSize(cvImg.cols, cvImg.rows);
	cv::Size imageSize;


	if (!bMaintainAspectRatio)
	{
		// Image should be the same size as the control's rectangle
		imageSize = winSize;
	}
	else
	{
		cv::Size newSize;

		AdjustAspectImageSize(origImageSize,
			winSize,
			imageSize);
	}
	if (bChange == TRUE)
	{
		bChange = FALSE;
		offsetX = (winSize.width - imageSize.width) / 2;
		offsetY = (winSize.height - imageSize.height) / 2;
	}
	// Resize the source to the size of the destination image if necessary
	Mat cvImgTmp;

	cv::resize(cvImg,
		cvImgTmp,
		imageSize,
		0,
		0,
		cv::INTER_AREA);
	if (nDlgID != 1001)
		return cvImgTmp;
	int X, Y;
	X = cvImgTmp.cols;
	Y = cvImgTmp.rows;
	CRect rc;
	GetDlgItem(nDlgID)->GetWindowRect(rc);
	if (X == rect.Width() && Y == rect.Height())
	{

	}
	/*else if (X == rect.Width())
	{
		int nDiff = (rect.bottom - Y) / 2;
		rc.left = rc.left;
		rc.right = rc.right;
		rc.top = rc.top + nDiff;
		rc.bottom = rc.bottom - nDiff;
		ScreenToClient(rc);
		m_ImageViewer.MoveWindow(rc, TRUE);
	}
	else if (Y == rect.Height())
	{
		int nDiff = (rect.right - X) / 2;
		rc.left = rc.left + nDiff;
		rc.right = rc.right - nDiff;
		rc.top = rc.top;
		rc.bottom = rc.bottom;
		ScreenToClient(rc);
		m_ImageViewer.MoveWindow(rc, TRUE);
	}*/

	ReleaseDC(pDC);
	return cvImgTmp;
}

Mat CKStitcherDlg::DrawPicToHDC1(cv::Mat  cvImg, UINT nDlgID, bool bMaintainAspectRatio /* =true*/)
{
	// Get the HDC handle information from the ID passed
	CDC* pDC = GetDlgItem(nDlgID)->GetDC();
	HDC  hDC = pDC->GetSafeHdc();

	CRect rect;
	GetDlgItem(nDlgID)->GetClientRect(rect);

	cv::Size winSize(rect.right, rect.bottom);

	// Calculate the size of the image that
	// will fit in the control rectangle.
	cv::Size origImageSize(cvImg.cols, cvImg.rows);
	cv::Size imageSize;


	if (!bMaintainAspectRatio)
	{
		// Image should be the same size as the control's rectangle
		imageSize = winSize;
	}
	else
	{
		cv::Size newSize;

		AdjustAspectImageSize(origImageSize,
			winSize,
			imageSize);
	}
	if (bChange == TRUE)
	{
		bChange = FALSE;
		offsetX = (winSize.width - imageSize.width) / 2;
		offsetY = (winSize.height - imageSize.height) / 2;
	}
	// Resize the source to the size of the destination image if necessary
	Mat cvImgTmp;

	cv::resize(cvImg,
		cvImgTmp,
		imageSize,
		0,
		0,
		cv::INTER_AREA);


	ReleaseDC(pDC);
	return cvImgTmp;
}

void CKStitcherDlg::DrawMatToHDC_CVV(Mat img, UINT ID)
{
	CDC *pDC = GetDlgItem(ID)->GetDC();
	HDC hDC = pDC->GetSafeHdc();
	CRect ClientRect;
	GetDlgItem(ID)->GetClientRect(&ClientRect);
	CvvImage cimg;
	IplImage ipl = cvIplImage(img);
	cimg.CopyOf(&ipl);
	cimg.DrawToHDC(hDC, &ClientRect);
	ReleaseDC(pDC);
}

void CKStitcherDlg::DrawMatToHDC(Mat img, UINT ID)
{
	//CRect ClientRect;
	CDC *pDC = GetDlgItem(ID)->GetDC();
	HDC hDC = pDC->GetSafeHdc();

	int stride = ((((img.cols * 24) + 31)  &  ~31) >> 3);

	// Allocate a buffer for our DIB bits
	uchar* pcDibBits = (uchar*)malloc(img.rows * stride);

	if (pcDibBits != NULL)
	{
		// Copy the raw pixel data over to our dibBits buffer.
		// NOTE: Can setup cvImgTmp to add the padding to skip this.
		for (int row = 0; row < img.rows; ++row)
		{
			// Get pointers to the beginning of the row on both buffers
			uchar* pcSrcPixel = img.ptr<uchar>(row);
			uchar* pcDstPixel = pcDibBits + (row * stride);

			// We can just use memcpy
			memcpy(pcDstPixel,
				pcSrcPixel,
				stride);
		}

		// Initialize the BITMAPINFO structure
		BITMAPINFO bitInfo;

		bitInfo.bmiHeader.biBitCount = 24;
		bitInfo.bmiHeader.biWidth = img.cols;
		bitInfo.bmiHeader.biHeight = -img.rows;
		bitInfo.bmiHeader.biPlanes = 1;
		bitInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitInfo.bmiHeader.biCompression = BI_RGB;
		bitInfo.bmiHeader.biClrImportant = 0;
		bitInfo.bmiHeader.biClrUsed = 0;
		bitInfo.bmiHeader.biSizeImage = 0;      //winSize.height * winSize.width * * 3;
		bitInfo.bmiHeader.biXPelsPerMeter = 0;
		bitInfo.bmiHeader.biYPelsPerMeter = 0;

		// Add header and OPENCV image's data to the HDC
		StretchDIBits(hDC,
			0,
			0,
			img.cols,
			img.rows,
			0,
			0,
			img.cols,
			img.rows,
			pcDibBits,
			&bitInfo,
			DIB_RGB_COLORS,
			SRCCOPY);

		free(pcDibBits);
	}
	ReleaseDC(pDC);
}

void CKStitcherDlg::AdjustAspectImageSize(const cv::Size& imageSize, const cv::Size& destSize, cv::Size& newSize)
{
	double destAspectRatio = float(destSize.width) / float(destSize.height);
	double imageAspectRatio = float(imageSize.width) / float(imageSize.height);

	if (imageAspectRatio > destAspectRatio)
	{
		// Margins on top/bottom
		newSize.width = destSize.width;
		newSize.height = int(imageSize.height  *
			(double(destSize.width) / double(imageSize.width)));
	}
	else
	{
		// Margins on left/right
		newSize.height = destSize.height;
		newSize.width = int(imageSize.width  *
			(double(destSize.height) / double(imageSize.height)));
	}
}

void CKStitcherDlg::OnBnClickedBtnCapture()
{
	bCapture = TRUE;
}
