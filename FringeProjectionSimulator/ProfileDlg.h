#pragma once


// ProfileDlg 대화 상자

class ProfileDlg : public CDialog
{
	DECLARE_DYNAMIC(ProfileDlg)

public:
	ProfileDlg(const IPVM::Image_16u_C1& src, CWnd* pParent);   // 표준 생성자입니다.
	ProfileDlg(const IPVM::Image_32f_C1& src, const bool useErrorProfile, CWnd* pParent);   // 표준 생성자입니다.
	virtual ~ProfileDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROFILE };
#endif

protected:
	IPVM::ImageView* m_imageView;
	IPVM::ProfileView* m_profileView;
	IPVM::Image_16u_C1* m_data16u;
	IPVM::Image_32f_C1* m_data32f;
	bool m_mouseLButtonDown;
	double m_rms;
	double m_minmax;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

	afx_msg void OnClose();
	afx_msg void OnNcDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	void CallbackOnMouseLButtonDown(const IPVM::Point_32f_C2& pt);
	void CallbackOnMouseMove(const IPVM::Point_32f_C2& pt);
	void CallbackOnMouseLButtonUp(const IPVM::Point_32f_C2& pt);

	void UpdateProfile(const IPVM::Point_32f_C2& pt);
};
