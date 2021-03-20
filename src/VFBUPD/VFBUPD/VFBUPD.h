
// VFBUPD.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "FloatFrame.h"

// CVFBUPDApp:
// See VFBUPD.cpp for the implementation of this class
//

class CVFBUPDApp : public CWinApp
{
public:
	CVFBUPDApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
public:
	CFloatFrame m_FloatFrame = {};
	BOOL m_bFloatFrame = FALSE;
	POINT m_ptFloat = { 0,0 };
	void ShowFloatFrame() {
		m_FloatFrame.ShowWindow(SW_SHOWNORMAL);
		m_FloatFrame.SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	void LoadFloatFramePosition() {
		CString text = GetProfileString(TEXT("POINT"), TEXT("FLOAT"), TEXT("(0,0)"));
		_stscanf_s(text, TEXT("(%d,%d)"), &m_ptFloat.x, &m_ptFloat.y);
		m_FloatFrame.SetWindowPos(&CWnd::wndTopMost, m_ptFloat.x, m_ptFloat.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		if (m_bFloatFrame == FALSE)
		{
			m_bFloatFrame = TRUE;
		}
	}
	void SaveFloatFramePosition()
	{
		if (m_bFloatFrame == TRUE)
		{
			CRect rc = { 0 };
			CString text(TEXT(""));
			m_FloatFrame.GetWindowRect(rc);
			m_ptFloat.x = rc.left;
			m_ptFloat.y = rc.top;
			text.Format(TEXT("(%d,%d)"), m_ptFloat.x, m_ptFloat.y);
			WriteProfileString(TEXT("POINT"), TEXT("FLOAT"), text);
		}
	}
};

extern CVFBUPDApp theApp;
