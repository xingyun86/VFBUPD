
// DlgProxy.h: header file
//

#pragma once

class CVFBUPDDlg;


// CVFBUPDDlgAutoProxy command target

class CVFBUPDDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(CVFBUPDDlgAutoProxy)

	CVFBUPDDlgAutoProxy();           // protected constructor used by dynamic creation

// Attributes
public:
	CVFBUPDDlg* m_pDialog;

// Operations
public:

// Overrides
	public:
	virtual void OnFinalRelease();

// Implementation
protected:
	virtual ~CVFBUPDDlgAutoProxy();

	// Generated message map functions

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CVFBUPDDlgAutoProxy)

	// Generated OLE dispatch map functions

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

