#pragma once

// CFloatFrame dialog

class CFloatFrame : public CDialogEx
{
	DECLARE_DYNAMIC(CFloatFrame)

public:
	CFloatFrame(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CFloatFrame();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_FLOAT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnMove(int x, int y);
	DECLARE_MESSAGE_MAP()
public:
	CFont m_font = {};
};
