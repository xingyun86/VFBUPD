
// VFBUPDDlg.h : header file
//

#pragma once

class CVFBUPDDlgAutoProxy;


// CVFBUPDDlg dialog
class CVFBUPDDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CVFBUPDDlg);
	friend class CVFBUPDDlgAutoProxy;

// Construction
public:
	CVFBUPDDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CVFBUPDDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VFBUPD_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	CVFBUPDDlgAutoProxy* m_pAutoProxy;
	HICON m_hIcon;

	BOOL CanExit();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
