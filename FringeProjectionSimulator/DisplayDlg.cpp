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
	constexpr int32_t kMinBlobLabelWidth = 70;
	constexpr int32_t kMinBlobEditWidth = 60;
	constexpr int32_t kTableHeight = 140;
	constexpr int32_t kThresholdMin = 0;
	constexpr int32_t kThresholdMax = 255;
	constexpr int32_t kThresholdDefault = 10;
	constexpr int32_t kMinBlobAreaMin = 0;
	constexpr int32_t kMinBlobAreaMax = 1000000;
	constexpr int32_t kMinBlobAreaDefault = 15;
	constexpr int32_t kBlobMaxNum = 10000;
	constexpr int32_t kThresholdScrollId = 1;
	constexpr int32_t kBlobTableId = 2;
	constexpr int32_t kMinBlobAreaEditId = 3;
}

DisplayDlg::DisplayDlg(const IPVM::Image_8u_C1& src, CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_PROFILE, pParent)
	, m_imageView(nullptr)
	, m_data8u(new IPVM::Image_8u_C1(src.GetSizeX(), src.GetSizeY()))
	, m_labelImage(new IPVM::Image_8u_C1(src.GetSizeX(), src.GetSizeY()))
	, m_blobDetection(new IPVM::BlobDetection)
	, m_blobInfos(new IPVM::BlobInfo[kBlobMaxNum])
	, m_threshold(kThresholdDefault)
	, m_minBlobArea(kMinBlobAreaDefault)
	, m_sortColumn(-1)
	, m_sortAscending(true)
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
	ON_EN_CHANGE(kMinBlobAreaEditId, &DisplayDlg::OnMinBlobAreaChange)
	ON_NOTIFY(LVN_COLUMNCLICK, kBlobTableId, &DisplayDlg::OnBlobTableColumnClick)
END_MESSAGE_MAP()


BOOL DisplayDlg::OnInitDialog()
{
	__super::OnInitDialog();

	SetWindowText(_T("Display"));

	CRect rtClient;
	GetClientRect(&rtClient);

	CRect rtImageView(rtClient);
	rtImageView.DeflateRect(0, 0, 0, kScrollbarHeight + kScrollbarPadding + kTableHeight + kScrollbarPadding);
	m_imageView = new IPVM::ImageView(GetSafeHwnd(), rtImageView);

	const CRect rtScrollbar(
		kScrollbarPadding,
		rtImageView.bottom + kScrollbarPadding,
		rtClient.right - kScrollbarPadding - kThresholdLabelWidth - kScrollbarPadding
		- kMinBlobLabelWidth - kScrollbarPadding - kMinBlobEditWidth - kScrollbarPadding,
		rtImageView.bottom + kScrollbarPadding + kScrollbarHeight);
	const CRect rtThresholdLabel(
		rtScrollbar.right + kScrollbarPadding,
		rtScrollbar.top,
		rtScrollbar.right + kScrollbarPadding + kThresholdLabelWidth,
		rtScrollbar.bottom);
	const CRect rtMinBlobLabel(
		rtThresholdLabel.right + kScrollbarPadding,
		rtScrollbar.top,
		rtThresholdLabel.right + kScrollbarPadding + kMinBlobLabelWidth,
		rtScrollbar.bottom);
	const CRect rtMinBlobAreaEdit(
		rtMinBlobLabel.right + kScrollbarPadding,
		rtScrollbar.top,
		rtMinBlobLabel.right + kScrollbarPadding + kMinBlobEditWidth,
		rtScrollbar.bottom);
	const CRect rtBlobTable(
		kScrollbarPadding,
		rtScrollbar.bottom + kScrollbarPadding,
		rtClient.right - kScrollbarPadding,
		rtScrollbar.bottom + kScrollbarPadding + kTableHeight);

	m_thresholdScroll.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rtScrollbar, this, kThresholdScrollId);
	m_thresholdScroll.SetScrollRange(kThresholdMin, kThresholdMax, FALSE);
	m_thresholdScroll.SetScrollPos(m_threshold, TRUE);

	m_thresholdLabel.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, rtThresholdLabel, this);
	UpdateThresholdLabel();

	m_minBlobAreaLabel.Create(_T("Min Area"), WS_CHILD | WS_VISIBLE | SS_CENTER, rtMinBlobLabel, this);
	m_minBlobAreaEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_RIGHT, rtMinBlobAreaEdit, this, kMinBlobAreaEditId);
	UpdateMinBlobAreaEdit();

	m_blobTable.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, rtBlobTable, this, kBlobTableId);
	m_blobTable.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_blobTable.InsertColumn(0, _T("Index"), LVCFMT_LEFT, 60);
	m_blobTable.InsertColumn(1, _T("Width"), LVCFMT_RIGHT, 80);
	m_blobTable.InsertColumn(2, _T("Height"), LVCFMT_RIGHT, 80);
	m_blobTable.InsertColumn(3, _T("Area"), LVCFMT_RIGHT, 90);

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
		CRect rtImageView(0, 0, cx, cy - kScrollbarHeight - kScrollbarPadding - kTableHeight - kScrollbarPadding);
		::MoveWindow(m_imageView->GetSafeHwnd(), rtImageView.left, rtImageView.top, rtImageView.Width(), rtImageView.Height(), TRUE);

		if (m_thresholdScroll.GetSafeHwnd())
		{
			const CRect rtScrollbar(
				kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding,
				cx - kScrollbarPadding - kMinBlobEditWidth - kScrollbarPadding
				- kMinBlobLabelWidth - kScrollbarPadding - kThresholdLabelWidth - kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding + kScrollbarHeight);
			m_thresholdScroll.MoveWindow(rtScrollbar);
		}

		if (m_thresholdLabel.GetSafeHwnd())
		{
			const CRect rtThresholdLabel(
				cx - kScrollbarPadding - kMinBlobEditWidth - kScrollbarPadding - kMinBlobLabelWidth - kScrollbarPadding - kThresholdLabelWidth,
				rtImageView.bottom + kScrollbarPadding,
				cx - kScrollbarPadding - kMinBlobEditWidth - kScrollbarPadding - kMinBlobLabelWidth - kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding + kScrollbarHeight);
			m_thresholdLabel.MoveWindow(rtThresholdLabel);
		}

		if (m_minBlobAreaLabel.GetSafeHwnd())
		{
			const CRect rtMinBlobLabel(
				cx - kScrollbarPadding - kMinBlobEditWidth - kScrollbarPadding - kMinBlobLabelWidth,
				rtImageView.bottom + kScrollbarPadding,
				cx - kScrollbarPadding - kMinBlobEditWidth - kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding + kScrollbarHeight);
			m_minBlobAreaLabel.MoveWindow(rtMinBlobLabel);
		}

		if (m_minBlobAreaEdit.GetSafeHwnd())
		{
			const CRect rtMinBlobEdit(
				cx - kScrollbarPadding - kMinBlobEditWidth,
				rtImageView.bottom + kScrollbarPadding,
				cx - kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding + kScrollbarHeight);
			m_minBlobAreaEdit.MoveWindow(rtMinBlobEdit);
		}

		if (m_blobTable.GetSafeHwnd())
		{
			const CRect rtBlobTable(
				kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding + kScrollbarHeight + kScrollbarPadding,
				cx - kScrollbarPadding,
				rtImageView.bottom + kScrollbarPadding + kScrollbarHeight + kScrollbarPadding + kTableHeight);
			m_blobTable.MoveWindow(rtBlobTable);
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

void DisplayDlg::OnMinBlobAreaChange()
{
	CString text;
	m_minBlobAreaEdit.GetWindowText(text);

	int32_t value = _ttoi(text);
	value = max(kMinBlobAreaMin, min(kMinBlobAreaMax, value));

	if (value != m_minBlobArea)
	{
		m_minBlobArea = value;
		UpdateDisplay();
	}
}

void DisplayDlg::OnBlobTableColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto* listView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int column = listView->iSubItem;

	if (column >= 1 && column <= 3)
	{
		if (m_sortColumn == column)
		{
			m_sortAscending = !m_sortAscending;
		}
		else
		{
			m_sortColumn = column;
			m_sortAscending = true;
		}

		m_blobTable.SortItems(&DisplayDlg::CompareBlobItems, reinterpret_cast<DWORD_PTR>(this));
	}

	*pResult = 0;
}

void DisplayDlg::UpdateDisplay()
{
	IPVM::Image_8u_C1 tempimage(m_data8u->GetSizeX(), m_data8u->GetSizeY());
	IPVM::Image_8u_C1 labelImage(m_data8u->GetSizeX(), m_data8u->GetSizeY());

	IPVM::ImageProcessing::BinarizeLess(*m_data8u, IPVM::Rect(*m_data8u), m_threshold, tempimage);

	int32_t blobCount = 0;
	m_blobDetection->DetectBlob_8Con(tempimage, IPVM::Rect(tempimage), m_minBlobArea, kBlobMaxNum, m_blobInfos, blobCount, labelImage);

	IPVM::ImageProcessing::Copy(labelImage, IPVM::Rect(labelImage), *m_labelImage);
	m_imageView->SetImage(*m_labelImage, IPVM::Rect(*m_labelImage));


	UpdateBlobTable(blobCount);
}

void DisplayDlg::UpdateThresholdLabel()
{
	CString text;
	text.Format(_T("%d"), m_threshold);
	m_thresholdLabel.SetWindowText(text);
}

void DisplayDlg::UpdateMinBlobAreaEdit()
{
	CString text;
	text.Format(_T("%d"), m_minBlobArea);
	m_minBlobAreaEdit.SetWindowText(text);
}

void DisplayDlg::UpdateBlobTable(int32_t blobCount)
{
	if (!m_blobTable.GetSafeHwnd())
	{
		return;
	}

	m_blobTable.SetRedraw(FALSE);
	m_blobTable.DeleteAllItems();

	for (int32_t index = 0; index < blobCount; ++index)
	{
		const auto& blobRect = m_blobInfos[index].m_roi;
		const int32_t width = blobRect.Width();
		const int32_t height = blobRect.Height();
		const int32_t area = width * height;

		CString text;
		text.Format(_T("%d"), index + 1);
		const int itemIndex = m_blobTable.InsertItem(index, text);
		m_blobTable.SetItemData(itemIndex, index);

		text.Format(_T("%d"), width);
		m_blobTable.SetItemText(index, 1, text);
		text.Format(_T("%d"), height);
		m_blobTable.SetItemText(index, 2, text);
		text.Format(_T("%d"), area);
		m_blobTable.SetItemText(index, 3, text);
	}

	if (m_sortColumn >= 1 && m_sortColumn <= 3)
	{
		m_blobTable.SortItems(&DisplayDlg::CompareBlobItems, reinterpret_cast<DWORD_PTR>(this));
	}

	m_blobTable.SetRedraw(TRUE);
	m_blobTable.Invalidate();
}

int CALLBACK DisplayDlg::CompareBlobItems(LPARAM leftItem, LPARAM rightItem, LPARAM sortParam)
{
	const auto* dialog = reinterpret_cast<const DisplayDlg*>(sortParam);
	const int32_t leftIndex = static_cast<int32_t>(leftItem);
	const int32_t rightIndex = static_cast<int32_t>(rightItem);

	const auto& leftRect = dialog->m_blobInfos[leftIndex].m_roi;
	const auto& rightRect = dialog->m_blobInfos[rightIndex].m_roi;

	int32_t leftValue = 0;
	int32_t rightValue = 0;

	switch (dialog->m_sortColumn)
	{
	case 1:
		leftValue = leftRect.Width();
		rightValue = rightRect.Width();
		break;
	case 2:
		leftValue = leftRect.Height();
		rightValue = rightRect.Height();
		break;
	case 3:
		leftValue = leftRect.Width() * leftRect.Height();
		rightValue = rightRect.Width() * rightRect.Height();
		break;
	default:
		break;
	}

	if (leftValue == rightValue)
	{
		return 0;
	}

	const int result = (leftValue < rightValue) ? -1 : 1;
	return dialog->m_sortAscending ? result : -result;
}