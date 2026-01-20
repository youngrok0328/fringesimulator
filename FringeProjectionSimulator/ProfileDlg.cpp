#include "pch.h"
#include "FringeProjectionSimulator.h"
#include "ProfileDlg.h"

#include "Algorithm/DataFitting.h"
#include "Algorithm/ImageProcessing.h"
#include "Algorithm/Polynomial.h"
#include "Base/ColorMapIndex.h"
#include "Types/Image_8u_C1.h"
#include "Types/Image_16u_C1.h"
#include "Types/Image_32f_C1.h"
#include "Types/Point_32f_C2.h"
#include "Types/Point_32f_C3.h"
#include "Types/Rect.h"
#include "Widget/ImageView.h"
#include "Widget/ProfileView.h"

#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ProfileDlg 대화 상자

IMPLEMENT_DYNAMIC(ProfileDlg, CDialog)

ProfileDlg::ProfileDlg(const IPVM::Image_16u_C1& src, CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_PROFILE, pParent)
	, m_imageView(nullptr)
	, m_profileView(nullptr)
	, m_data16u(new IPVM::Image_16u_C1(src.GetSizeX(), src.GetSizeY()))
	, m_data32f(nullptr)
	, m_mouseLButtonDown(false)
	, m_rms(0.)
	, m_minmax(0.)
{
	IPVM::ImageProcessing::Copy(src, IPVM::Rect(src), *m_data16u);
}

ProfileDlg::ProfileDlg(const IPVM::Image_32f_C1& src, const bool useErrorProfile, CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_PROFILE, pParent)
	, m_imageView(nullptr)
	, m_profileView(nullptr)
	, m_data16u(nullptr)
	, m_data32f(new IPVM::Image_32f_C1(src.GetSizeX(), src.GetSizeY()))
	, m_mouseLButtonDown(false)
{
	if (useErrorProfile)
	{
		const auto imageSizeX = src.GetSizeX();
		const auto imageSizeY = src.GetSizeY();

		std::vector<IPVM::Point_32f_C3> points;
		points.reserve(imageSizeX * imageSizeY);

		const auto imageCenterX = static_cast<float>((imageSizeX - 1) * 0.5f);
		const auto imageCenterY = static_cast<float>((imageSizeY - 1) * 0.5f);

		for (int32_t y = 0; y < imageSizeY; y++)
		{
			const auto mem_src = src.GetMem(0, y);

			const float fy = y - imageCenterY;

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const float fx = x - imageCenterX;

				if (mem_src[x] != NOISE_VALUE_32F)
				{
					points.emplace_back(fx, fy, mem_src[x]);
				}
			}
		}

		static const int32_t order = 4;

		std::vector<double> coefs(IPVM::Polynomial::GetCoefCount(order));

		IPVM::DataFitting::FitToNthOrderPolynomial3DSurface(points.size(), &points[0], order, &coefs[0]);

		const auto* coefs_mem = &coefs[0];

		double sqrSum = 0.;
		int32_t sumCount = 0;

		double min = 1000000000000.;
		double max = -1000000000000.;

		for (int32_t y = 0; y < imageSizeY; y++)
		{
			const auto mem_src = src.GetMem(0, y);
			const auto mem_phase = m_data32f->GetMem(0, y);

			const float fy = y - imageCenterY;

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const float fx = x - imageCenterX;

				if (mem_src[x] != NOISE_VALUE_32F)
				{
					const auto diff = mem_src[x] - IPVM::Polynomial::GetValue4th(fx, fy, coefs_mem);

					sqrSum += diff * diff;
					sumCount++;

					if (diff < min)
					{
						min = diff;
					}

					if (diff > max)
					{
						max = diff;
					}

					mem_phase[x] = static_cast<float>(diff);
				}
				else
				{
					mem_phase[x] = NOISE_VALUE_32F;
				}
			}
		}

		m_rms = ::sqrt(sqrSum / sumCount);
		m_minmax = max - min;
	}
	else
	{
		IPVM::ImageProcessing::Copy(src, IPVM::Rect(src), *m_data32f);
	}
}

ProfileDlg::~ProfileDlg()
{
	delete m_data32f;
	delete m_data16u;
	delete m_profileView;
	delete m_imageView;
}

void ProfileDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ProfileDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_NCDESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// ProfileDlg 메시지 처리기


BOOL ProfileDlg::OnInitDialog()
{
	__super::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	CRect rtClient;
	GetClientRect(&rtClient);

	CRect rtImageView(rtClient);
	rtImageView.DeflateRect(0, 0, 0, rtClient.Height() / 4);

	m_imageView = new IPVM::ImageView(GetSafeHwnd(), rtImageView);

	if (m_data16u)
	{
		IPVM::Image_8u_C1 temp8u(m_data16u->GetSizeX(), m_data16u->GetSizeY());

		IPVM::ImageProcessing::Scale(*m_data16u, IPVM::Rect(*m_data16u), 1 / 16., 0, temp8u);

		m_imageView->SetImage(temp8u, IPVM::Rect(temp8u));
	}
	else if (m_data32f)
	{
		m_imageView->SetImage(*m_data32f, IPVM::Rect(*m_data32f), NOISE_VALUE_32F, IPVM::ColorMapIndex::Rainbow);
	}

	m_imageView->RegisterCallback_MouseLButtonDown(GetSafeHwnd(), this, 
		[](const int32_t id, void* userData, const IPVM::Point_32f_C2& pt)
		{
			UNREFERENCED_PARAMETER(id);
			return ((ProfileDlg*)userData)->CallbackOnMouseLButtonDown(pt);
		}
	);
	m_imageView->RegisterCallback_MouseMove(GetSafeHwnd(), this, 
		[](const int32_t id, void* userData, const IPVM::Point_32f_C2& pt)
		{
			UNREFERENCED_PARAMETER(id);
			return ((ProfileDlg*)userData)->CallbackOnMouseMove(pt);
		}
	);
	m_imageView->RegisterCallback_MouseLButtonUp(GetSafeHwnd(), this, 
		[](const int32_t id, void* userData, const IPVM::Point_32f_C2& pt)
		{
			UNREFERENCED_PARAMETER(id);
			return ((ProfileDlg*)userData)->CallbackOnMouseLButtonUp(pt);
		}
	);

	CRect rtProfileView(rtClient);
	rtProfileView.DeflateRect(0, rtImageView.bottom, 0, 0);

	m_profileView = new IPVM::ProfileView(GetSafeHwnd(), rtProfileView);

	if (m_data16u)
	{
		const auto imageSizeX = m_data16u->GetSizeX();
		const auto imageSizeY = m_data16u->GetSizeY();

		std::vector<IPVM::Point_32f_C2> data;
		data.reserve(imageSizeX);

		const auto mem_phase = m_data16u->GetMem(0, imageSizeY / 2);

		for (int32_t x = 0; x < imageSizeX; x++)
		{
			data.emplace_back(static_cast<float>(x), static_cast<float>(mem_phase[x]));
		}

		m_profileView->SetData(0, &data[0], imageSizeX, NOISE_VALUE_32F, RGB(255, 0, 0), RGB(255, 0, 0));
	}
	else if (m_data32f)
	{
		const auto imageSizeX = m_data32f->GetSizeX();
		const auto imageSizeY = m_data32f->GetSizeY();

		std::vector<IPVM::Point_32f_C2> data;
		data.reserve(imageSizeX);

		const auto mem_phase = m_data32f->GetMem(0, imageSizeY / 2);

		for (int32_t x = 0; x < imageSizeX; x++)
		{
			data.emplace_back(static_cast<float>(x), mem_phase[x]);
		}

		m_profileView->SetData(0, &data[0], imageSizeX, NOISE_VALUE_32F, RGB(255, 0, 0), RGB(255, 0, 0));
	}

	if (m_rms > 0)
	{
		CString strTitle;
		GetWindowText(strTitle);

		strTitle.AppendFormat(_T(" :: RMS Error %.3lf, MinMax %.3lf"), m_rms, m_minmax);

		SetWindowText(strTitle);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void ProfileDlg::OnClose()
{
	__super::OnClose();

	DestroyWindow();
}


void ProfileDlg::OnNcDestroy()
{
	__super::OnNcDestroy();

	delete this;
}

void ProfileDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (m_imageView->GetSafeHwnd())
	{
		CRect rtImageView(0, 0, cx, 3 * cy / 4);
		::MoveWindow(m_imageView->GetSafeHwnd(), rtImageView.left, rtImageView.top, rtImageView.Width(), rtImageView.Height(), TRUE);

		CRect rtProfileView(0, rtImageView.bottom, cx, cy);
		::MoveWindow(m_profileView->GetSafeHwnd(), rtProfileView.left, rtProfileView.top, rtProfileView.Width(), rtProfileView.Height(), TRUE);
	}
}

void ProfileDlg::CallbackOnMouseLButtonDown(const IPVM::Point_32f_C2& pt)
{
	m_mouseLButtonDown = true;

	UpdateProfile(pt);
}

void ProfileDlg::CallbackOnMouseMove(const IPVM::Point_32f_C2& pt)
{
	if (m_mouseLButtonDown)
	{
		UpdateProfile(pt);
	}
}

void ProfileDlg::CallbackOnMouseLButtonUp(const IPVM::Point_32f_C2& pt)
{
	UNREFERENCED_PARAMETER(pt);

	m_mouseLButtonDown = false;
}

void ProfileDlg::UpdateProfile(const IPVM::Point_32f_C2& pt)
{
	const auto pty = static_cast<int32_t>(pt.m_y + 0.5f);

	if (m_data16u)
	{
		const auto imageSizeX = m_data16u->GetSizeX();
		const auto imageSizeY = m_data16u->GetSizeY();

		if (pty >= 0 && pty < imageSizeY)
		{
			std::vector<IPVM::Point_32f_C2> data;
			data.reserve(imageSizeX);

			const auto mem_phase = m_data16u->GetMem(0, pty);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				data.emplace_back(static_cast<float>(x), static_cast<float>(mem_phase[x]));
			}

			m_profileView->SetData(0, &data[0], imageSizeX, NOISE_VALUE_32F, RGB(255, 0, 0), RGB(255, 0, 0));
		}
	}
	else if (m_data32f)
	{
		const auto imageSizeX = m_data32f->GetSizeX();
		const auto imageSizeY = m_data32f->GetSizeY();

		if (pty >= 0 && pty < imageSizeY)
		{
			std::vector<IPVM::Point_32f_C2> data;
			data.reserve(imageSizeX);

			const auto mem_phase = m_data32f->GetMem(0, pty);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				data.emplace_back(static_cast<float>(x), mem_phase[x]);
			}

			m_profileView->SetData(0, &data[0], imageSizeX, NOISE_VALUE_32F, RGB(255, 0, 0), RGB(255, 0, 0));
		}
	}
}
