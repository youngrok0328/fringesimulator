#include "pch.h"
#include "framework.h"
#include "FringeProjectionSimulator.h"
#include "FringeProjectionSimulatorDlg.h"

#include "CommandDlg.h"
#include "FringeGenerator.h"
#include "ImageLogger.h"
#include "MISImageLoader.h"
#include "Parameter.h"
#include "ParameterDlg.h"
#include "Process1D.h"
#include "Process2D.h"
#include "ProfileDlg.h"

#include "Algorithm/FastFourierTransform1D.h"
#include "Algorithm/ImageProcessing.h"
#include "Types/Image_8u_C1.h"
#include "Types/Image_16u_C1.h"
#include "Types/Image_32f_C1.h"
#include "Types/BlobInfo.h"
#include "Widget/AsyncProgress.h"
#include "Widget/ImageView.h"
#include "Algorithm/BlobDetection.h"
#include "Types/Rect.h"

#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define BLOB_MAX_NUM	10000

FringeProjectionSimulatorDlg::FringeProjectionSimulatorDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_MAIN, pParent)
	, m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
	, m_imageLogger(new ImageLogger)
	, m_parameter(new Parameter)
	, m_imageView(nullptr)
	, m_commandDlg(nullptr)
	, m_parameterDlg(nullptr)
	, m_inputImage(new IPVM::Image_8u_C1)
	, m_inputHorImage(new IPVM::Image_16u_C1)
	, m_firstROISetting(true)
	, m_blobDetection(new IPVM::BlobDetection)
	, m_blobInfos(new IPVM::BlobInfo[BLOB_MAX_NUM])
{
}

FringeProjectionSimulatorDlg::~FringeProjectionSimulatorDlg()
{
	delete m_inputImage;
	delete m_inputHorImage;
	delete m_parameterDlg;
	delete m_commandDlg;
	delete m_imageView;
	delete m_parameter;
	delete m_imageLogger;
	delete[] m_blobInfos;
	delete m_blobDetection;
}

void FringeProjectionSimulatorDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(FringeProjectionSimulatorDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, OnDockingPaneNotify)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_MESSAGE(UM_LOAD_IMAGE, OnLoadImage)
	ON_MESSAGE(UM_SAVE_IMAGE, OnSaveImage)
	ON_MESSAGE(UM_GENERATE_FRINGE, OnGenerateFringe)
	ON_MESSAGE(UM_PROCESS_FRINGE, OnProcessFringe)
	ON_MESSAGE(UM_VIEW_IMAGE, OnViewImage)
	ON_MESSAGE(UM_VIEW_PROFILE, OnViewProfile)
	ON_MESSAGE(UM_AUTOGENERATE_FRINGE, OnAutoGenerateFringe)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// FringeProjectionSimulatorDlg 메시지 처리기

BOOL FringeProjectionSimulatorDlg::OnInitDialog()
{
	__super::OnInitDialog();

	ShowWindow(SW_MAXIMIZE);

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	m_imageView = new IPVM::ImageView(GetSafeHwnd(), 0);
	m_imageView->SetBackgroundColor(RGB(0, 60, 30));

	VERIFY(m_paneManager.InstallDockingPanes(this));
	m_paneManager.SetTheme(xtpPaneThemeOffice2003);
	m_paneManager.UseSplitterTracker(FALSE);
	m_paneManager.SetShowContentsWhileDragging(TRUE);
	m_paneManager.SetAlphaDockingContext(TRUE);

	// Create docking panes.
	CXTPDockingPane* pwndPane0 = m_paneManager.CreatePane(IDR_PANE_COMMAND, XTP_DPI(CRect(0, 0, 400, 150)), xtpPaneDockLeft);
	pwndPane0->SetOptions(xtpPaneNoCloseable);
	CXTPDockingPane* pwndPane1 = m_paneManager.CreatePane(IDR_PANE_PARAMETER, XTP_DPI(CRect(0, 0, 400, 400)), xtpPaneDockBottom, pwndPane0);
	pwndPane1->SetOptions(xtpPaneNoCloseable);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void FringeProjectionSimulatorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		__super::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR FringeProjectionSimulatorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void FringeProjectionSimulatorDlg::Resize(int32_t cx, int32_t cy)
{
	CRect rcClient(0, 0, cx, cy);
	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, 0, 0, &rcClient);
	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &rcClient, &rcClient);

	if (m_imageView->GetSafeHwnd())
	{
		::MoveWindow(m_imageView->GetSafeHwnd(), rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), TRUE);
	}
}

void FringeProjectionSimulatorDlg::SetFirstROI()
{
	const auto imageCenterX = m_inputImage->GetSizeX() / 2;
	const auto imageCenterY = m_inputImage->GetSizeY() / 2;

	m_imageView->ROISet(_T("ROI"), _T("ROI"), IPVM::Rect(imageCenterX - 100, imageCenterY - 100, imageCenterX + 101, imageCenterX + 101), RGB(255, 255, 255), 20);
	m_imageView->ROIShow();

	m_firstROISetting = false;
}

LRESULT FringeProjectionSimulatorDlg::OnAutoSaveImage(int nFovNum)
{
	//
	//OPENFILENAME ofn;
	//memset(&ofn, 0, sizeof(ofn));

	//std::vector<TCHAR> pathName(32768, L'\0');
	//
	//ofn.lStructSize = sizeof(ofn);
	//ofn.hwndOwner = m_hWnd;
	////ofn.lpstrFilter = _T("All Supported Image Files (*.bmp;*.jpg;*.jpeg;*.png)\0*.bmp;*.jpg;*.jpeg;*.png\0BMP Image Files (*.bmp)\0*.bmp\0PNG Image Files (*.png)\0*.png\0JPEG Image Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0\0");
	//ofn.lpstrFilter = _T("BMP Image Files (*.bmp)\0*.bmp\0\0");
	//ofn.lpstrFile = &pathName[0];
	//ofn.nMaxFile = 32768;
	//ofn.lpstrDefExt = _T("*.bmp");
	//ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;

	//
	//int32_t size_x = m_parameter->m_imageSizeX;
	//int32_t size_y = m_parameter->m_imageSizeY;	
	//

	//IPVM::Image_16u_C1 temp_verimage(size_x, size_y);
	//temp_verimage.FillZero();

	//IPVM::Image_16u_C1 temp_Horimage(size_x, size_y);
	//temp_Horimage.FillZero();

	//int32_t offset_x = (m_inputImage->GetSizeX() - size_x) / 2;
	//int32_t offset_y = (m_inputImage->GetSizeY() - size_y) / 2;

	//IPVM::Rect src_rect{ offset_x, offset_y, size_x + offset_x, size_y + offset_y };

	//IPVM::ImageProcessing::Copy(*m_inputImage, src_rect, IPVM::Rect(temp_verimage), temp_verimage);
	//IPVM::ImageProcessing::Copy(*m_inputHorImage, src_rect, IPVM::Rect(temp_Horimage), temp_Horimage);
	//
	//CString strMainPath;
	//strMainPath.Format(_T("D:\\AutosaveImage"));
	//CreateDirectory(strMainPath, nullptr);

	//int nDepth = m_parameter->m_gaussianShapeDepth_mm * 1000;
	//int nFWHM = m_parameter->m_gaussianShapeFWHM_mm * 1000 ;
	//
	//CString strSpec;
	//strSpec.Format(_T("D:\\AutosaveImage\\Depth%dum_FWHM%dum"),nDepth, nFWHM);
	//CreateDirectory(strSpec, nullptr);

	//CString filepath;
	//filepath.Format(_T("%s\\%d.bmp"), strSpec, nFovNum);

	//MISImageLoader::Save_V4(filepath, temp_Horimage, temp_verimage, true, 1);

	return 0;
}

void FringeProjectionSimulatorDlg::OnClose()
{
	m_parameterDlg->DestroyWindow();

	__super::OnClose();
}

void FringeProjectionSimulatorDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	Resize(cx, cy);
}

LRESULT FringeProjectionSimulatorDlg::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pPane = (CXTPDockingPane*)lParam;
		if (!pPane->IsValid())
		{
			if (pPane->GetID() == IDR_PANE_COMMAND)
			{
				m_commandDlg = new CommandDlg(this);
				m_commandDlg->Create(IDD_COMMAND, this);

				pPane->Attach(m_commandDlg);
			}
			else if (pPane->GetID() == IDR_PANE_PARAMETER)
			{
				m_parameterDlg = new ParameterDlg(*m_parameter, this);
				m_parameterDlg->Create(IDD_PARAMETER, this);

				pPane->Attach(m_parameterDlg);
			}
		}

		return TRUE; // handled
	}

	return FALSE;
}

LRESULT FringeProjectionSimulatorDlg::OnKickIdle(WPARAM, LPARAM)
{
	if (m_paneManager.GetSafeHwnd())
		m_paneManager.UpdatePanes();

	return 0;
}

LRESULT FringeProjectionSimulatorDlg::OnLoadImage(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(lparam);

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));

	std::vector<TCHAR> pathName(32768, L'\0');

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = _T("All Supported Image Files (*.bmp;*.jpg;*.jpeg;*.png)\0*.bmp;*.jpg;*.jpeg;*.png\0BMP Image Files (*.bmp)\0*.bmp\0PNG Image Files (*.png)\0*.png\0JPEG Image Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0\0");
	ofn.lpstrFile = &pathName[0];
	ofn.nMaxFile = 32768;
	ofn.lpstrDefExt = _T("*.bmp");
	ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if (::GetOpenFileName(&ofn) == FALSE)
	{
		return 0;
	}

	auto customBitmapVersion = MISImageLoader::GetFileVersion(ofn.lpstrFile);

	int32_t horSize = 0;
	int32_t verSize = 0;
	int32_t bpp = 0;

	IPVM::Image::InfoFrom(ofn.lpstrFile, horSize, verSize, bpp);

	IPVM::Image_8u_C1 tempImage;
	tempImage.LoadFrom(ofn.lpstrFile);
	
	m_inputImage->Create(horSize, verSize);
	m_inputImage->FillZero();
	
	IPVM::ImageProcessing::Copy(tempImage,IPVM::Rect(tempImage), *m_inputImage);
	
	m_imageView->SetImage(*m_inputImage);
	
	return 0;
}

LRESULT FringeProjectionSimulatorDlg::OnSaveImage(WPARAM wparam, LPARAM lparam) 
{
	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(lparam);

	//OPENFILENAME ofn;
	//memset(&ofn, 0, sizeof(ofn));

	//std::vector<TCHAR> pathName(32768, L'\0');

	//ofn.lStructSize = sizeof(ofn);
	//ofn.hwndOwner = m_hWnd;
	////ofn.lpstrFilter = _T("All Supported Image Files (*.bmp;*.jpg;*.jpeg;*.png)\0*.bmp;*.jpg;*.jpeg;*.png\0BMP Image Files (*.bmp)\0*.bmp\0PNG Image Files (*.png)\0*.png\0JPEG Image Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0\0");
	//ofn.lpstrFilter = _T("BMP Image Files (*.bmp)\0*.bmp\0\0");
	//ofn.lpstrFile = &pathName[0];
	//ofn.nMaxFile = 32768;
	//ofn.lpstrDefExt = _T("*.bmp");
	//ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 

	//if (::GetSaveFileName(&ofn) == FALSE)
	//{
	//	return 0;
	//}

	//int32_t size_x = m_parameter->m_imageSizeX;
	//int32_t size_y = m_parameter->m_imageSizeY;

	//IPVM::Image_16u_C1 temp_verimage(size_x, size_y);
	//temp_verimage.FillZero();

	//IPVM::Image_16u_C1 temp_Horimage(size_x, size_y);
	//temp_Horimage.FillZero();

	//int32_t offset_x = (m_inputImage->GetSizeX() - size_x) / 2;
	//int32_t offset_y = (m_inputImage->GetSizeY() - size_y) / 2;	

	//IPVM::Rect src_rect{offset_x, offset_y, size_x + offset_x, size_y + offset_y};

	//IPVM::ImageProcessing::Copy(*m_inputImage, src_rect, IPVM::Rect(temp_verimage), temp_verimage);
	//IPVM::ImageProcessing::Copy(*m_inputHorImage, src_rect, IPVM::Rect(temp_Horimage), temp_Horimage);
	//	
	//MISImageLoader::Save_V3(ofn.lpstrFile, temp_Horimage, temp_verimage, true);
	
	return 0;
}

LRESULT FringeProjectionSimulatorDlg::OnGenerateFringe(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(lparam);

	/*IPVM::AsyncProgress progress(_T("Generating"));

	m_imageLogger->Reset();
		
	FringeGenerator::Generate(*m_parameter, *m_inputImage, *m_inputHorImage);

	m_commandDlg->UpdateImageList(*m_imageLogger);

	if (m_firstROISetting)
	{
		SetFirstROI();
	}*/

	return 0;
}

LRESULT FringeProjectionSimulatorDlg::OnProcessFringe(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(lparam);

	
	int32_t blobCount = 0;

	IPVM::Image_8u_C1 tempimage(m_inputImage->GetSizeX(), m_inputImage->GetSizeY());
	IPVM::Image_8u_C1 labelImage(m_inputImage->GetSizeX(), m_inputImage->GetSizeY());

	IPVM::ImageProcessing::BinarizeLess(*m_inputImage, IPVM::Rect(*m_inputImage), 10, tempimage);
	
	m_blobDetection->DetectBlob_8Con(tempimage, IPVM::Rect(tempimage), 15, BLOB_MAX_NUM, m_blobInfos, blobCount, labelImage);

	if (blobCount == 0)
	{
		// blob 실패 이유 찾기
	}
		
	m_imageView->ImageOverlayClear();

	m_imageView->SetImage(labelImage);
	
	return 0;
}

LRESULT FringeProjectionSimulatorDlg::OnViewImage(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(wparam);

	//if (lparam <= 0)
	//{
	//	IPVM::Image_8u_C1 temp8u(m_inputImage->GetSizeX(), m_inputImage->GetSizeY());

	//	IPVM::ImageProcessing::Scale(*m_inputImage, IPVM::Rect(*m_inputImage), 1 / 16., 0, temp8u);

	//	m_imageView->SetImage(temp8u);

	//	return 0;
	//}

	//const int32_t index = static_cast<int32_t>(lparam - 1);

	//auto* image = m_imageLogger->GetImage(index);

	//if (dynamic_cast<IPVM::Image_32f_C1*>(image))
	//{
	//	auto* image32fC1 = dynamic_cast<IPVM::Image_32f_C1*>(image);

	//	m_imageView->SetImage(*image32fC1, NOISE_VALUE_32F, m_imageLogger->GetColorMapIndex(index));
	//}
	//else if (dynamic_cast<IPVM::Image_16u_C1*>(image))
	//{
	//	auto* image16uC1 = dynamic_cast<IPVM::Image_16u_C1*>(image);

	//	IPVM::Image_8u_C1 temp8u(image16uC1->GetSizeX(), image16uC1->GetSizeY());

	//	IPVM::ImageProcessing::Scale(*image16uC1, IPVM::Rect(*image16uC1), 1 / 16., 0, temp8u);

	//	m_imageView->SetImage(temp8u);
	//}

	return 0;
}

LRESULT FringeProjectionSimulatorDlg::OnViewProfile(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(wparam);

	IPVM::Rect roi;
	m_imageView->ROIGet(_T("ROI"), roi);

	const int32_t index = static_cast<int32_t>(lparam - 1);

	/*if (index < 0)
	{
		roi &= IPVM::Rect(*m_inputImage);

		if (!roi.IsRectEmpty() && !roi.IsRectNull())
		{
			(new ProfileDlg(IPVM::Image_16u_C1(*m_inputImage, roi), this))->Create(IDD_PROFILE, this);
		}
	}
	else
	{
		const auto image = m_imageLogger->GetImage(index);

		if (const auto concreteImage16uC1 = dynamic_cast<IPVM::Image_16u_C1*>(image))
		{
			roi &= IPVM::Rect(*concreteImage16uC1);

			if (!roi.IsRectEmpty() && !roi.IsRectNull())
			{
				(new ProfileDlg(IPVM::Image_16u_C1(*concreteImage16uC1, roi), this))->Create(IDD_PROFILE, this);
			}
		}
		else if (const auto concreteImage32fC1 = dynamic_cast<IPVM::Image_32f_C1*>(image))
		{
			roi &= IPVM::Rect(*concreteImage32fC1);

			if (!roi.IsRectEmpty() && !roi.IsRectNull())
			{
				(new ProfileDlg(IPVM::Image_32f_C1(*concreteImage32fC1, roi), m_imageLogger->GetUseErrorProfile(index), this))->Create(IDD_PROFILE, this);
			}
		}
	}*/

	return 0;
}

LRESULT FringeProjectionSimulatorDlg::OnAutoGenerateFringe(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(lparam);

	//int nImageCnt = m_parameter->m_saveImageCount;
	//double FWHM_Inc = m_parameter->m_FWHM_IncStep_mm;
	//
	//double DepthStartValue_mm = m_parameter->m_gaussianShapeDepth_mm;
	//double FWHMStartValue_mm = m_parameter->m_gaussianShapeFWHM_mm;
	//
	//int DepthStep = 5;
	//int FWHMStep = m_parameter->m_FWHM_IncStepCount;
	//
	//CString message;
	//message.Format(_T("[%d]장 패턴이미지를  저장하시겠습니까?"), nImageCnt);
	//
	//if (MessageBox(message, _T("메시지"), MB_YESNO) != IDYES)
	//{
	//	return false;
	//}
	//
	//IPVM::AsyncProgress progress(_T("Auto Generating"));

	//for (int x = 0; x < DepthStep; x++)
	//{
	//	if (x > 2)
	//	{
	//		if (x == 3)
	//		{
	//			m_parameter->m_gaussianShapeDepth_mm = 0.035;
	//		}
	//		else
	//		{
	//			m_parameter->m_gaussianShapeDepth_mm = 0.050;
	//		}
	//	}
	//	else
	//	{
	//		m_parameter->m_gaussianShapeDepth_mm = DepthStartValue_mm + (0.005 * x);
	//	}
	//	
	//	
	//	for (int y = 0; y < FWHMStep; y++)
	//	{
	//		
	//		
	//		if(y>21)
	//		{
	//			double CurVal = m_parameter->m_gaussianShapeFWHM_mm;

	//			m_parameter->m_gaussianShapeFWHM_mm = CurVal + 0.45;

	//			
	//		}
	//		else
	//		{
	//			m_parameter->m_gaussianShapeFWHM_mm = FWHMStartValue_mm + (FWHM_Inc * y);
	//		}
	//		
	//		
	//		for (int i = 0; i < nImageCnt; i++)
	//		{
	//			FringeGenerator::Generate(*m_parameter, *m_inputImage, *m_inputHorImage);

	//			int nFovNum = i + 1; //fov 1번부터 시작

	//			OnAutoSaveImage(nFovNum);

	//		}
	//	}
	//}
	
	return 0;
	
}



BOOL FringeProjectionSimulatorDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'S' && GetKeyState(VK_CONTROL) < 0 )
	{
		m_commandDlg->AutoGenerateIMageSave();
		
		return TRUE;
	}


	return CDialog::PreTranslateMessage(pMsg);
}
