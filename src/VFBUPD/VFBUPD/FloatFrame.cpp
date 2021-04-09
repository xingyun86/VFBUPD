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
	m_font.CreatePointFont(96, TEXT("宋体"));
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
	ON_WM_NCHITTEST()
	ON_WM_MOVE()
	ON_STN_DBLCLK(IDC_STATIC_STATUS, &CFloatFrame::OnStnDblclickStaticStatus)
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

LRESULT CFloatFrame::OnNcHitTest(CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CRect rect;
	GetWindowRect(&rect);
	CRect rect1 = rect;
	rect1.DeflateRect(6, 6, -6, -6);
	rect1.NormalizeRect();
	if (point.x <= rect.left + 2)
		return HTLEFT;
	else if (point.x >= rect.right - 2)
		return HTRIGHT;
	else if (point.y <= rect.top + 2)
		return HTTOP;
	else if (point.y >= rect.bottom - 2)
		return HTBOTTOM;
	else if (point.x <= rect.left + 6 && point.y <= rect.top + 6)
		return HTTOPLEFT;
	else if (point.x >= rect.right - 6 && point.y <= rect.top + 6)
		return HTTOPRIGHT;
	else if (point.x <= rect.left + 6 && point.y >= rect.bottom - 6)
		return HTBOTTOMLEFT;
	else if (point.x >= rect.right - 6 && point.y >= rect.bottom - 6)
		return HTBOTTOMRIGHT;
	else if (!rect.IsRectEmpty())
	{
		LRESULT uRet = CWnd::OnNcHitTest(point);
		uRet = (uRet == HTCLIENT) ? HTCAPTION : uRet;
		return uRet;
	}
	else
	{
		{
			CRect rect = {};
			CRect rectOk = {};
			CRect rectCancel = {};
			GetClientRect(&rect);
			GetDlgItem(IDOK)->GetClientRect(rectOk);
			GetDlgItem(IDCANCEL)->GetClientRect(rectCancel);
			rect.right = rect.Width() - (rectOk.Width() + rectCancel.Width());
			rect.bottom = rectOk.Height();
			ClientToScreen(&rect);
			return rect.PtInRect(point) ? HTCAPTION : CDialog::OnNcHitTest(point);   //鼠标如果在客户区，将其当作标题栏
		}
		return CDialogEx::OnNcHitTest(point);
	}
	return 0;
	//return CDialogEx::OnNcHitTest(point);
}


void CFloatFrame::OnMove(int x, int y)
{
	CDialogEx::OnMove(x, y);

	// TODO: Add your message handler code here
	theApp.SaveFloatFramePosition();
}


void CFloatFrame::OnStnDblclickStaticStatus()
{
	// TODO: Add your control notification handler code here
	//if (theApp.m_dlg->IsWindowVisible() == FALSE)
	{
		theApp.m_dlg->ShowWindow(SW_SHOWNORMAL);
	}
}
