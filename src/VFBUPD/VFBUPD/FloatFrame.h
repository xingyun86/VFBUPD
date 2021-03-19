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
	DECLARE_MESSAGE_MAP()
public:
	CFont m_font = {};
	void ShowTopMost()
	{
		::SetWindowPos(this->GetSafeHwnd(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	void ShowNoTopMost()
	{
		::SetWindowPos(this->GetSafeHwnd(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
};
