
// VFBUPDDlg.h : header file
//

#pragma once

class CVFBUPDDlgAutoProxy;


// CVFBUPDDlg dialog
class CVFBUPDDlg : public CDialogEx, public WindowHandle
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
	virtual void OnProgress(DWORD dwTotalBytes, DWORD dwExistBytes);
	afx_msg void OnBnClickedButtonShow();
	DECLARE_MESSAGE_MAP()

private:
	TSTRING AppDir() {
		TSTRING filePath(MAX_PATH, TEXT('\0'));
		GetModuleFileName(NULL, (LPTSTR)filePath.data(), (DWORD)filePath.size());
		return filePath.substr(0, filePath.rfind(TEXT('\\')));
	}
	TSTRING AppDir_Posix() {
		TSTRING filePath(MAX_PATH, TEXT('\0'));
		GetModuleFileName(NULL, (LPTSTR)filePath.data(), (DWORD)filePath.size());
		for (auto& it : filePath) { if (it == TEXT('\\')) { it = TEXT('/'); } }
		return filePath.substr(0, filePath.rfind(TEXT('/')));
	}
public:
	TSTRING m_fileUrl = TEXT("");
	TSTRING m_savePath = AppDir() + TEXT("\\files\\");
	std::shared_ptr<std::thread> m_taskThread = nullptr;
};
