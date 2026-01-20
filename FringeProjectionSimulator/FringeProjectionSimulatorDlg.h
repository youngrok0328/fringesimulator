
// FringeProjectionSimulatorDlg.h: 헤더 파일
//

#pragma once

class CommandDlg;
class ImageLogger;
class ParameterDlg;
struct Parameter;

// FringeProjectionSimulatorDlg 대화 상자
class FringeProjectionSimulatorDlg : public CDialog
{
	// 생성입니다.
public:
	FringeProjectionSimulatorDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	~FringeProjectionSimulatorDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


	// 구현입니다.
protected:
	HICON m_hIcon;

	CXTPDockingPaneManager m_paneManager;

	ImageLogger* m_imageLogger;
	Parameter* m_parameter;
	IPVM::ImageView* m_imageView;
	CommandDlg* m_commandDlg;
	ParameterDlg* m_parameterDlg;

	IPVM::Image_8u_C1* m_inputImage;
	IPVM::Image_16u_C1* m_inputHorImage;

	//blob
	IPVM::BlobDetection* m_blobDetection;
	IPVM::BlobInfo* m_blobInfos;
	
	bool m_firstROISetting;

	void Resize(int32_t cx, int32_t cy);
	void SetFirstROI();
	LRESULT OnAutoSaveImage(int nFovNum);

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg LRESULT OnLoadImage(WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT OnSaveImage(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnGenerateFringe(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnProcessFringe(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnViewImage(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnViewProfile(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnAutoGenerateFringe(WPARAM wparam, LPARAM lparam);

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
