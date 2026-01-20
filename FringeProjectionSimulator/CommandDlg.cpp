#include "pch.h"
#include "FringeProjectionSimulator.h"
#include "CommandDlg.h"

#include "ImageLogger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CommandDlg, CDialog)

CommandDlg::CommandDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_COMMAND, pParent)
	, m_parentHwnd(pParent->GetSafeHwnd())
{

}

CommandDlg::~CommandDlg()
{
}

void CommandDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_LOAD_IMAGE, m_buttonLoadImage);
    DDX_Control(pDX, IDC_BUTTON_SAVE_IMAGE, m_buttonSaveImage);
	DDX_Control(pDX, IDC_BUTTON_GENERATE_FRINGE, m_buttonGenerateFringe);
	DDX_Control(pDX, IDC_BUTTON_PROCESS_FRINGE, m_buttonProcessFringe);
	DDX_Control(pDX, IDC_BUTTON_DISPLAY, m_buttonDisplay);
	DDX_Control(pDX, IDC_LIST_IMAGES, m_images);
}


BEGIN_MESSAGE_MAP(CommandDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_GENERATE_FRINGE, &CommandDlg::OnBnClickedButtonGenerateFringe)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_IMAGE, &CommandDlg::OnBnClickedButtonLoadImage)
    ON_BN_CLICKED(IDC_BUTTON_SAVE_IMAGE, &CommandDlg::OnBnClickedButtonSaveImage)
	ON_BN_CLICKED(IDC_BUTTON_PROCESS_FRINGE, &CommandDlg::OnBnClickedButtonProcessFringe)
	ON_BN_CLICKED(IDC_BUTTON_DISPLAY, &CommandDlg::OnBnClickedButtonDisplay)
	ON_LBN_SELCHANGE(IDC_LIST_IMAGES, &CommandDlg::OnLbnSelchangeListImages)
	ON_LBN_DBLCLK(IDC_LIST_IMAGES, &CommandDlg::OnLbnDblclkListImages)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CommandDlg 메시지 처리기
BOOL CommandDlg::OnInitDialog()
{
	__super::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CommandDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (m_buttonGenerateFringe.GetSafeHwnd() == NULL)
	{
		return;
	}

	const CRect rtClient(1, 1, cx - 1, cy - 1);

	CRect rtButton;
	m_buttonLoadImage.GetWindowRect(&rtButton);

	const int32_t buttonHeight = rtButton.Height();
	const int32_t controlGap = 1;

	CButton *buttons[] = { &m_buttonGenerateFringe, &m_buttonLoadImage, &m_buttonSaveImage, &m_buttonProcessFringe, &m_buttonDisplay };
	const int32_t buttonCount = _countof(buttons);

	const int32_t buttonWidth = (rtClient.Width() - (buttonCount + 1) * controlGap) / buttonCount;

	for (int32_t i = 0; i < buttonCount; i++)
	{
		buttons[i]->MoveWindow(CRect((i + 1) * controlGap + i * buttonWidth, 1 * controlGap, (i + 1) * (controlGap + buttonWidth), 1 * controlGap + buttonHeight));
	}

	m_images.MoveWindow(CRect(controlGap, 2 * controlGap + buttonHeight, rtClient.right - controlGap, rtClient.bottom - controlGap));
}

void CommandDlg::OnBnClickedButtonLoadImage()
{
	::PostMessage(m_parentHwnd, UM_LOAD_IMAGE, 0, 0);
}

void CommandDlg::OnBnClickedButtonSaveImage()
{
	::PostMessage(m_parentHwnd, UM_SAVE_IMAGE, 0, 0);
}


void CommandDlg::OnBnClickedButtonGenerateFringe()
{
	::PostMessage(m_parentHwnd, UM_GENERATE_FRINGE, 0, 0);
}

void CommandDlg::OnBnClickedButtonProcessFringe()
{
	::PostMessage(m_parentHwnd, UM_PROCESS_FRINGE, 0, 0);
}

void CommandDlg::OnLbnSelchangeListImages()
{
	int32_t curSel = m_images.GetCurSel();

	::PostMessage(m_parentHwnd, UM_VIEW_IMAGE, 0, curSel);
}

void CommandDlg::OnLbnDblclkListImages()
{
	int32_t curSel = m_images.GetCurSel();

	::PostMessage(m_parentHwnd, UM_VIEW_PROFILE, 0, curSel);
}

void CommandDlg::UpdateImageList(const ImageLogger& imageLogger)
{
	m_images.ResetContent();

	m_images.AddString(_T(">> Input Image <<"));

	for (int32_t i = 0; i < imageLogger.GetCount(); i++)
	{
		m_images.AddString(imageLogger.GetTitle(i));
	}

	m_images.SetCurSel(imageLogger.GetCount());

	OnLbnSelchangeListImages();
}

void CommandDlg::AutoGenerateIMageSave()
{
	::PostMessage(m_parentHwnd, UM_AUTOGENERATE_FRINGE, 0, 0);
}

void CommandDlg::OnBnClickedButtonDisplay()
{
	::PostMessage(m_parentHwnd, UM_DISPLAY, 0, 0);
	//
}
