#pragma once

// DisplayDlg 대화 상자

class DisplayDlg : public CDialog
{
	DECLARE_DYNAMIC(DisplayDlg)

public:
	DisplayDlg(const IPVM::Image_8u_C1& src, CWnd* pParent);   // 표준 생성자입니다.
	virtual ~DisplayDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROFILE };
#endif

protected:
	IPVM::ImageView* m_imageView;
	IPVM::Image_8u_C1* m_data8u;
	IPVM::Image_8u_C1* m_labelImage;
	IPVM::BlobDetection* m_blobDetection;
	IPVM::BlobInfo* m_blobInfos;
	CScrollBar m_thresholdScroll;
	int m_threshold;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

	afx_msg void OnClose();
	afx_msg void OnNcDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	void UpdateDisplay();
};