#include "pch.h"
#include "FringeProjectionSimulator.h"
#include "ParameterDlg.h"
#include "Parameter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(ParameterDlg, CDialog)

ParameterDlg::ParameterDlg(Parameter& parameter, CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_PARAMETER, pParent)
	, m_parameter(parameter)
	, m_property(new CXTPPropertyGrid)
{

}

ParameterDlg::~ParameterDlg()
{
	delete m_property;
}

void ParameterDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ParameterDlg, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// ParameterDlg 메시지 처리기


BOOL ParameterDlg::OnInitDialog()
{
	__super::OnInitDialog();

	CRect rtClient;
	GetClientRect(&rtClient);

	m_property->Create(rtClient, this, 0);

	if (auto* category = m_property->AddCategory(_T("기본")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("이미지 가로 크기"), m_parameter.m_imageSizeX, (long*)&m_parameter.m_imageSizeX)))
		{

		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("이미지 세로 크기"), m_parameter.m_imageSizeY, (long*)&m_parameter.m_imageSizeY)))
		{

		}
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("자동 저장 이미지 장수"), m_parameter.m_saveImageCount, (long*)&m_parameter.m_saveImageCount)))
		{

		}
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("FWHM 증가 크기"), m_parameter.m_FWHM_IncStep_mm, _T("%.3lf mm"), &m_parameter.m_FWHM_IncStep_mm)))
		{

		}
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("FWHM 증가 Step수"), m_parameter.m_FWHM_IncStepCount, (long*)&m_parameter.m_FWHM_IncStepCount)))
		{

		}
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("픽셀 분해능"), m_parameter.m_px2mm, _T("%.3lf mm/px"), &m_parameter.m_px2mm)))
		{

		}

//		category->Expand();
	}

	if (auto* category = m_property->AddCategory(_T("프로젝션 조명")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("조명 입사각"), m_parameter.m_illuminationIncidence_theta_degree, _T("%.0lf deg"), &m_parameter.m_illuminationIncidence_theta_degree)))
		{
			item->SetDescription(_T("구면 좌표계에서의 조명 입사각"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("조명 방향"), m_parameter.m_illuminationIncidence_phi_degree, _T("%.0lf deg"), &m_parameter.m_illuminationIncidence_phi_degree)))
		{
			item->SetDescription(_T("구면 좌표계에서의 조명 방향"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("조명 밝기"), m_parameter.m_illuminationIntensity_gv, _T("%.0lf gv"), &m_parameter.m_illuminationIntensity_gv)))
		{
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("격자 피치"), m_parameter.m_illuminationGratingPitch_mm, _T("%.3lf mm/px"), &m_parameter.m_illuminationGratingPitch_mm)))
		{
			item->SetDescription(_T("텔레센트릭 프로젝션을 가정한 격자 피치"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("위상 옵셋"), m_parameter.m_illuminationGratingPhaseOffset_deg, _T("%.0lf deg"), &m_parameter.m_illuminationGratingPhaseOffset_deg)))
		{
		}

//		category->Expand();
	}

	if (auto* category = m_property->AddCategory(_T("배경 형상 (2차 곡면)")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("XX 의 계수"), m_parameter.m_backgroundShape_xx, _T("%.6lf"), &m_parameter.m_backgroundShape_xx)))
		{
			item->SetDescription(_T("z = axx + bxy + cyy + dx + ey + f 에서의 a"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("XY 의 계수"), m_parameter.m_backgroundShape_xy, _T("%.6lf"), &m_parameter.m_backgroundShape_xy)))
		{
			item->SetDescription(_T("z = axx + bxy + cyy + dx + ey + f 에서의 b"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("YY 의 계수"), m_parameter.m_backgroundShape_yy, _T("%.6lf"), &m_parameter.m_backgroundShape_yy)))
		{
			item->SetDescription(_T("z = axx + bxy + cyy + dx + ey + f 에서의 c"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("X 의 계수"), m_parameter.m_backgroundShape_x, _T("%.6lf"), &m_parameter.m_backgroundShape_x)))
		{
			item->SetDescription(_T("z = axx + bxy + cyy + dx + ey + f 에서의 d"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("Y 의 계수"), m_parameter.m_backgroundShape_y, _T("%.6lf"), &m_parameter.m_backgroundShape_y)))
		{
			item->SetDescription(_T("z = axx + bxy + cyy + dx + ey + f = 0 에서의 e"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("옵셋"), m_parameter.m_backgroundShape_z0, _T("%.6lf"), &m_parameter.m_backgroundShape_z0)))
		{
			item->SetDescription(_T("z = axx + bxy + cyy + dx + ey + f = 0 에서의 f"));
		}

//		category->Expand();
	}

	if (auto* category = m_property->AddCategory(_T("물체 형상 (가우스 함수)")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("위치 X"), m_parameter.m_gaussainShapePositionX_mm, _T("%.3lf mm"), &m_parameter.m_gaussainShapePositionX_mm)))
		{
			item->SetDescription(_T("가우스 함수 피크의 X 위치"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("위치 Y"), m_parameter.m_gaussainShapePositionY_mm, _T("%.3lf mm"), &m_parameter.m_gaussainShapePositionY_mm)))
		{
			item->SetDescription(_T("가우스 함수 피크의 Y 위치"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("높이 혹은 깊이"), m_parameter.m_gaussianShapeDepth_mm, _T("%.3lf mm"), &m_parameter.m_gaussianShapeDepth_mm)))
		{
			item->SetDescription(_T("가우스 함수의 피크값"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("FWHM"), m_parameter.m_gaussianShapeFWHM_mm, _T("%.3lf mm"), &m_parameter.m_gaussianShapeFWHM_mm)))
		{
			item->SetDescription(_T("가우스 함수의 Full Width Half Maximum"));
		}

		//category->Expand();
	}

	if (auto* category = m_property->AddCategory(_T("표면 반사")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("표면각 X 무작위화 최대값"), m_parameter.m_surfaceNormalRandomLimitX_degree, _T("%.1lf deg"), &m_parameter.m_surfaceNormalRandomLimitX_degree)))
		{
			item->SetDescription(_T("거친 금속면을 모사하기 위해 사용\r\n표면의 X 축 방향으로의 기울기 변화량"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("표면각 Y 무작위화 최대값"), m_parameter.m_surfaceNormalRandomLimitY_degree, _T("%.1lf deg"), &m_parameter.m_surfaceNormalRandomLimitY_degree)))
		{
			item->SetDescription(_T("거친 금속면을 모사하기 위해 사용\r\n표면의 Y 축 방향으로의 기울기 변화량"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("표면 반사광 확산도"), m_parameter.m_reflectionDistribution_rad, _T("%.1lf rad"), &m_parameter.m_reflectionDistribution_rad)))
		{
			item->SetDescription(_T("표면이 경면에 가까울수록 작은 값을 사용"));
		}

//		category->Expand();
	}

	if (auto* category = m_property->AddCategory(_T("Point spread function")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("Gaussian function sigma"), m_parameter.m_pointSpreadFunctionSigma, _T("%.2lf px"), &m_parameter.m_pointSpreadFunctionSigma)))
		{
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("Gaussian function kernel size"), m_parameter.m_pointSpreadFunctionKernelSize, (long*)&m_parameter.m_pointSpreadFunctionKernelSize)))
		{
		}

		//category->Expand();
	}

	if (auto* category = m_property->AddCategory(_T("기타")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("Intensity offset"), m_parameter.m_modulationIntensityOffset_gv, _T("%.1lf gv"), &m_parameter.m_modulationIntensityOffset_gv)))
		{
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("Poisson noise gain"), m_parameter.m_poissonNoiseLimit_ratio, _T("%.2lf"), &m_parameter.m_poissonNoiseLimit_ratio)))
		{
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemDouble(_T("Gaussian noise sigma"), m_parameter.m_gaussianNoiseSigma_gv, _T("%.1lf gv"), &m_parameter.m_gaussianNoiseSigma_gv)))
		{
		}

		//category->Expand();
	}

	if (auto* category = m_property->AddCategory(_T("FTM Parameters")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemBool(_T("1D FFT 사용"), m_parameter.m_fft_1D, &m_parameter.m_fft_1D)))
		{
			item->SetDescription(_T("FFT 에 1D 알고리즘을 사용하기"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemBool(_T("이미지 가장자리 미러링"), m_parameter.m_fft_boundaryMirroring, &m_parameter.m_fft_boundaryMirroring)))
		{
			item->SetDescription(_T("FFT 하기 전에 가장자리 미러링 수행"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemBool(_T("Hanning 윈도우 적용"), m_parameter.m_fft_useHanningWindow, &m_parameter.m_fft_useHanningWindow)))
		{
			item->SetDescription(_T("FFT 하기 전에 Hanning 윈도우 적용"));
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("Carrier frequency - X"), m_parameter.m_fft_carrierFrequencyX, (long*)&m_parameter.m_fft_carrierFrequencyX)))
		{

		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("Frequency bandwidth - X"), m_parameter.m_fft_frequencyBandWidthX, (long*)&m_parameter.m_fft_frequencyBandWidthX)))
		{

		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("Frequency bandwidth - Y"), m_parameter.m_fft_frequencyBandWidthY, (long*)&m_parameter.m_fft_frequencyBandWidthY)))
		{

		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("Butterworth filter order"), m_parameter.m_fft_butterworthFilterOrder, (long*)&m_parameter.m_fft_butterworthFilterOrder)))
		{

		}

		//category->Expand();
	}

	if (auto* category = m_property->AddCategory(_T("Debugging")))
	{
		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemBool(_T("Load vertical fringe image"), m_parameter.m_loadVerticalFringeImage, &m_parameter.m_loadVerticalFringeImage)))
		{
		}

		if (auto* item = category->AddChildItem(new CXTPPropertyGridItemNumber(_T("Fringe reconstruction gain"), m_parameter.m_fft_fringeReconstructionGain, (long*)&m_parameter.m_fft_fringeReconstructionGain)))
		{

		}

		//category->Expand();
	}

	m_property->SetViewDivider(0.5);
	
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void ParameterDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (m_property->GetSafeHwnd())
	{
		::MoveWindow(m_property->GetSafeHwnd(), 1, 1, cx - 2, cy - 2, TRUE);
	}
}
