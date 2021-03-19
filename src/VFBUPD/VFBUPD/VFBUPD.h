
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
};

extern CVFBUPDApp theApp;
