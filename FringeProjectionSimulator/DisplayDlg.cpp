#include "pch.h"
#include "FringeProjectionSimulator.h"
#include "DisplayDlg.h"

#include "Algorithm/ImageProcessing.h"
#include "Algorithm/BlobDetection.h"
#include "Types/BlobInfo.h"
#include "Types/Image_8u_C1.h"
#include "Types/Rect.h"
#include "Widget/ImageView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// DisplayDlg 대화 상자

IMPLEMENT_DYNAMIC(DisplayDlg, CDialog)

namespace
{
	constexpr int32_t kScrollbarHeight = 18;
	constexpr int32_t kScrollbarPadding = 4;
	constexpr int32_t kThresholdLabelWidth = 60;
	constexpr int32_t kThresholdMin = 0;
	constexpr int32_t kThresholdMax = 255;
	constexpr int32_t kThresholdDefault = 10;
	constexpr int32_t kBlobMaxNum = 10000;
}

DisplayDlg::DisplayDlg(const IPVM::Image_8u_C1& src, CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_PROFILE, pParent)
	, m_imageView(nullptr)
	, m_data8u(new IPVM::Image_8u_C1(src.GetSizeX(), src.GetSizeY()))
	, m_labelImage(new IPVM::Image_8u_C1(src.GetSizeX(), src.GetSizeY()))
	, m_blobDetection(new IPVM::BlobDetection)
	, m_blobInfos(new IPVM::BlobInfo[kBlobMaxNum])
	, m_threshold(kThresholdDefault)
{
	IPVM::ImageProcessing::Copy(src, IPVM::Rect(src), *m_data8u);
}

DisplayDlg::~DisplayDlg()
{
	delete[] m_blobInfos;
	delete m_blobDetection;
	delete m_labelImage;
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
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


BOOL DisplayDlg::OnInitDialog()
{
	__super::OnInitDialog();

	SetWindowText(_T("Display"));

	CRect rtClient;
	GetClientRect(&rtClient);

	CRect rtImageView(rtClient);
	rtImageView.DeflateRect(0, 0, 0, kScrollbarHeight + kScrollbarPadding);

	m_imageView = new IPVM::ImageView(GetSafeHwnd(), rtImageView);

	const CRect rtScrollbar(
		kScrollbarPadding,
		rtImageView.bottom + kScrollbarPadding,
		rtClient.right - kScrollbarPadding - kThresholdLabelWidth - kScrollbarPadding,
		rtImageView.bottom + kScrollbarPadding + kScrollbarHeight);
	const CRect rtThresholdLabel(
		rtScrollbar.right + kScrollbarPadding,
		rtScrollbar.top,
		rtScrollbar.right + kScrollbarPadding + kThresholdLabelWidth,
		rtScrollbar.bottom);

	m_thresholdScroll.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rtScrollbar, this, 1);
	m_thresholdScroll.SetScrollRange(kThresholdMin, kThresholdMax, FALSE);
	m_thresholdScroll.SetScrollPos(m_threshold, TRUE);

	m_thresholdLabel.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, rtThresholdLabel, this);
	UpdateThresholdLabel();

	UpdateDisplay();

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
		CRect rtImageView(0, 0, cx, cy - kScrollbarHeight - kScrollbarPadding);
		::MoveWindow(m_imageView->GetSafeHwnd(), rtImageView.left, rtImageView.top, rtImageView.Width(), rtImageView.Height(), TRUE);

		if (m_thresholdScroll.GetSafeHwnd())
		{
			const CRect rtScrollbar(
				kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding,
				cx - kScrollbarPadding - kThresholdLabelWidth - kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding + kScrollbarHeight);
			m_thresholdScroll.MoveWindow(rtScrollbar);
		}

		if (m_thresholdLabel.GetSafeHwnd())
		{
			const CRect rtThresholdLabel(
				cx - kScrollbarPadding - kThresholdLabelWidth,
				rtImageView.bottom + kScrollbarPadding,
				cx - kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding + kScrollbarHeight);
			m_thresholdLabel.MoveWindow(rtThresholdLabel);
		}
	}
}

void DisplayDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar != &m_thresholdScroll)
	{
		__super::OnHScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	int newPos = m_thresholdScroll.GetScrollPos();

	switch (nSBCode)
	{
	case SB_LINELEFT:
		newPos -= 1;
		break;
	case SB_LINERIGHT:
		newPos += 1;
		break;
	case SB_PAGELEFT:
		newPos -= 10;
		break;
	case SB_PAGERIGHT:
		newPos += 10;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newPos = static_cast<int>(nPos);
		break;
	default:
		break;
	}

	newPos = max(kThresholdMin, min(kThresholdMax, newPos));

	if (newPos != m_threshold)
	{
		m_threshold = newPos;
		m_thresholdScroll.SetScrollPos(m_threshold, TRUE);
		UpdateThresholdLabel();
		UpdateDisplay();
	}
}

void DisplayDlg::UpdateDisplay()
{
	IPVM::Image_8u_C1 tempimage(m_data8u->GetSizeX(), m_data8u->GetSizeY());
	IPVM::Image_8u_C1 labelImage(m_data8u->GetSizeX(), m_data8u->GetSizeY());

	IPVM::ImageProcessing::BinarizeLess(*m_data8u, IPVM::Rect(*m_data8u), m_threshold, tempimage);

	int32_t blobCount = 0;
	m_blobDetection->DetectBlob_8Con(tempimage, IPVM::Rect(tempimage), 15, kBlobMaxNum, m_blobInfos, blobCount, labelImage);

	IPVM::ImageProcessing::Copy(labelImage, IPVM::Rect(labelImage), *m_labelImage);
	m_imageView->SetImage(*m_labelImage, IPVM::Rect(*m_labelImage));
}

void DisplayDlg::UpdateThresholdLabel()
{
	CString text;
	text.Format(_T("%d"), m_threshold);
	m_thresholdLabel.SetWindowText(text);
}