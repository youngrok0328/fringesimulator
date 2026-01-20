#pragma once


// CommandDlg 대화 상자
class ImageLogger;

class CommandDlg : public CDialog
{
	DECLARE_DYNAMIC(CommandDlg)

public:
	CommandDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CommandDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_COMMAND };
#endif

protected:
	HWND m_parentHwnd;
	CButton m_buttonGenerateFringe;
	CButton m_buttonLoadImage;
    CButton m_buttonSaveImage;
	CButton m_buttonProcessFringe;
	CButton m_buttonDisplay;

	CListBox m_images;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedButtonGenerateFringe();
	afx_msg void OnBnClickedButtonLoadImage();
	afx_msg void OnBnClickedButtonSaveImage();
	afx_msg void OnBnClickedButtonProcessFringe();
	afx_msg void OnLbnSelchangeListImages();
	afx_msg void OnLbnDblclkListImages();

public:
	void UpdateImageList(const ImageLogger& imageLogger);
	void AutoGenerateIMageSave();
	afx_msg void OnBnClickedButtonDisplay();
};
