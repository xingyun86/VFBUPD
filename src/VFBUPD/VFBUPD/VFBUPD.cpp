
// VFBUPD.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "VFBUPD.h"
#include "VFBUPDDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVFBUPDApp

BEGIN_MESSAGE_MAP(CVFBUPDApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CVFBUPDApp construction

CVFBUPDApp::CVFBUPDApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CVFBUPDApp object

CVFBUPDApp theApp;

const GUID CDECL BASED_CODE _tlid =
		{0x1bb61e91,0xfa87,0x4fdd,{0xa6,0x51,0xb4,0xe4,0x47,0x52,0x22,0x5c}};
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


// CVFBUPDApp initialization

BOOL CVFBUPDApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	// Parse command line for automation or reg/unreg switches.
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// App was launched with /Embedding or /Automation switch.
	// Run app as automation server.
	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		// Register class factories via CoRegisterClassObject().
		COleTemplateServer::RegisterAll();
	}
	// App was launched with /Unregserver or /Unregister switch.  Remove
	// entries from the registry.
	else if (cmdInfo.m_nShellCommand == CCommandLineInfo::AppUnregister)
	{
		COleObjectFactory::UpdateRegistryAll(FALSE);
		AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor);
		return FALSE;
	}
	// App was launched standalone or with other switches (e.g. /Register
	// or /Regserver).  Update registry entries, including typelibrary.
	else
	{
		COleObjectFactory::UpdateRegistryAll();
		AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid);
		if (cmdInfo.m_nShellCommand == CCommandLineInfo::AppRegister)
			return FALSE;
	}
#ifdef _DEBUG
	if (AllocConsole())
	{
		freopen(("CONIN$"), ("rb"), stdin);
		freopen(("CONOUT$"), ("wb"), stdout);
		freopen(("CONOUT$"), ("wb"), stderr);
	}
#endif
	CVFBUPDDlg dlg;
	m_FloatFrame.Create(IDD_DIALOG_FLOAT);
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}
	
	m_FloatFrame.DestroyWindow();

	// Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#ifdef _DEBUG
	FreeConsole();
#endif
#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CVFBUPDApp::ExitInstance()
{
	AfxOleTerm(FALSE);

	return CWinApp::ExitInstance();
}
