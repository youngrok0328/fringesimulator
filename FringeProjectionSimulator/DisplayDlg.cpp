#include "pch.h"
#include "FringeProjectionSimulator.h"
#include "DisplayDlg.h"

#include "Algorithm/ImageProcessing.h"
#include "Types/Image_8u_C1.h"
#include "Types/Rect.h"
#include "Widget/ImageView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// DisplayDlg 대화 상자

IMPLEMENT_DYNAMIC(DisplayDlg, CDialog)

DisplayDlg::DisplayDlg(const IPVM::Image_8u_C1& src, CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_PROFILE, pParent)
	, m_imageView(nullptr)
	, m_data8u(new IPVM::Image_8u_C1(src.GetSizeX(), src.GetSizeY()))
{
	IPVM::ImageProcessing::Copy(src, IPVM::Rect(src), *m_data8u);
}

DisplayDlg::~DisplayDlg()
{
	delete m_data8u;
	delete m_imageView;
}

void DisplayDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(DisplayDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_NCDESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()


BOOL DisplayDlg::OnInitDialog()
{
	__super::OnInitDialog();

	SetWindowText(_T("Display"));

	CRect rtClient;
	GetClientRect(&rtClient);

	m_imageView = new IPVM::ImageView(GetSafeHwnd(), rtClient);
	m_imageView->SetImage(*m_data8u, IPVM::Rect(*m_data8u));

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void DisplayDlg::OnClose()
{
	__super::OnClose();

	DestroyWindow();
}

void DisplayDlg::OnNcDestroy()
{
	__super::OnNcDestroy();

	delete this;
}

void DisplayDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (m_imageView && m_imageView->GetSafeHwnd())
	{
		CRect rtImageView(0, 0, cx, cy);
		::MoveWindow(m_imageView->GetSafeHwnd(), rtImageView.left, rtImageView.top, rtImageView.Width(), rtImageView.Height(), TRUE);
	}
}