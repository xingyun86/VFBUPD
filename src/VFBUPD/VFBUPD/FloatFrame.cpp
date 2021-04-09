// FloatFrame.cpp : implementation file
//

#include "pch.h"
#include "VFBUPD.h"
#include "afxdialogex.h"
#include "FloatFrame.h"


// CFloatFrame dialog

IMPLEMENT_DYNAMIC(CFloatFrame, CDialogEx)

CFloatFrame::CFloatFrame(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_FLOAT, pParent)
{
	m_font.CreatePointFont(128, TEXT("ËÎÌו"));
}

CFloatFrame::~CFloatFrame()
{
	m_font.DeleteObject();
}

void CFloatFrame::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFloatFrame, CDialogEx)
	ON_WM_MOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


// CFloatFrame message handlers

BOOL CFloatFrame::OnInitDialog() 
{
	::SetWindowLongPtr(this->GetSafeHwnd(), GWL_EXSTYLE, GetWindowLongPtr(this->GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_TOPMOST | WS_EX_TOOLWINDOW & (~WS_EX_APPWINDOW));
	GetDlgItem(IDC_STATIC_STATUS)->SetFont(&m_font);
	GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(TEXT(""));
	theApp.LoadFloatFramePosition();
	return FALSE;
}

void CFloatFrame::OnMove(int x, int y)
{
	CDialogEx::OnMove(x, y);

	// TODO: Add your message handler code here
	theApp.SaveFloatFramePosition();
}


void CFloatFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
	CDialogEx::OnLButtonDown(nFlags, point);
}


void CFloatFrame::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	theApp.m_dlg->ShowWindow(SW_SHOWNORMAL);

	CDialogEx::OnLButtonDblClk(nFlags, point);
}
