
// DlgProxy.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "VFBUPD.h"
#include "DlgProxy.h"
#include "VFBUPDDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVFBUPDDlgAutoProxy

IMPLEMENT_DYNCREATE(CVFBUPDDlgAutoProxy, CCmdTarget)

CVFBUPDDlgAutoProxy::CVFBUPDDlgAutoProxy()
{
	EnableAutomation();

	// To keep the application running as long as an automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();

	// Get access to the dialog through the application's
	//  main window pointer.  Set the proxy's internal pointer
	//  to point to the dialog, and set the dialog's back pointer to
	//  this proxy.
	ASSERT_VALID(AfxGetApp()->m_pMainWnd);
	if (AfxGetApp()->m_pMainWnd)
	{
		ASSERT_KINDOF(CVFBUPDDlg, AfxGetApp()->m_pMainWnd);
		if (AfxGetApp()->m_pMainWnd->IsKindOf(RUNTIME_CLASS(CVFBUPDDlg)))
		{
			m_pDialog = reinterpret_cast<CVFBUPDDlg*>(AfxGetApp()->m_pMainWnd);
			m_pDialog->m_pAutoProxy = this;
		}
	}
}

CVFBUPDDlgAutoProxy::~CVFBUPDDlgAutoProxy()
{
	// To terminate the application when all objects created with
	// 	with automation, the destructor calls AfxOleUnlockApp.
	//  Among other things, this will destroy the main dialog
	if (m_pDialog != nullptr)
		m_pDialog->m_pAutoProxy = nullptr;
	AfxOleUnlockApp();
}

void CVFBUPDDlgAutoProxy::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CVFBUPDDlgAutoProxy, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CVFBUPDDlgAutoProxy, CCmdTarget)
END_DISPATCH_MAP()

// Note: we add support for IID_IVFBUPD to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {6208ec37-994a-4d1d-9553-0061fb539c03}
static const IID IID_IVFBUPD =
{0x6208ec37,0x994a,0x4d1d,{0x95,0x53,0x00,0x61,0xfb,0x53,0x9c,0x03}};

BEGIN_INTERFACE_MAP(CVFBUPDDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CVFBUPDDlgAutoProxy, IID_IVFBUPD, Dispatch)
END_INTERFACE_MAP()

// The IMPLEMENT_OLECREATE2 macro is defined in pch.h of this project
// {202ee070-8ce7-4a31-ac15-b2f30760e817}
IMPLEMENT_OLECREATE2(CVFBUPDDlgAutoProxy, "VFBUPD.Application", 0x202ee070,0x8ce7,0x4a31,0xac,0x15,0xb2,0xf3,0x07,0x60,0xe8,0x17)


// CVFBUPDDlgAutoProxy message handlers
