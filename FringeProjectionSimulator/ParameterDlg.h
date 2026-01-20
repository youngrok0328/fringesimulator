#pragma once

class CXTPPropertyGrid;
class FringeProjectionSimulatorDlg;
struct Parameter;

class ParameterDlg : public CDialog
{
	DECLARE_DYNAMIC(ParameterDlg)

public:
	ParameterDlg(Parameter &parameter, CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~ParameterDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PARAMETER };
#endif

protected:
	Parameter& m_parameter;
	CXTPPropertyGrid* m_property;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
