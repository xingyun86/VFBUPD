
// VFBUPDDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "VFBUPD.h"
#include "VFBUPDDlg.h"
#include "DlgProxy.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
	EnableActiveAccessibility();
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CVFBUPDDlg dialog


IMPLEMENT_DYNAMIC(CVFBUPDDlg, CDialogEx);

CVFBUPDDlg::CVFBUPDDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VFBUPD_DIALOG, pParent)
{
	EnableActiveAccessibility();
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = nullptr;
}

CVFBUPDDlg::~CVFBUPDDlg()
{
	// If there is an automation proxy for this dialog, set
	//  its back pointer to this dialog to null, so it knows
	//  the dialog has been deleted.
	if (m_pAutoProxy != nullptr)
		m_pAutoProxy->m_pDialog = nullptr;
}

void CVFBUPDDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVFBUPDDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SHOW, &CVFBUPDDlg::OnBnClickedButtonShow)
END_MESSAGE_MAP()


// CVFBUPDDlg message handlers

BOOL CVFBUPDDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.
	// 
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	SetWindowText(TEXT("FatCatTool-V1.0"));
	SetDlgItemText(IDC_BUTTON_SHOW, TEXT("FilePath"));
	SetDlgItemText(IDOK, TEXT("Download"));
	SetDlgItemText(IDCANCEL, TEXT("Close"));
	SetDlgItemText(IDC_STATIC_DOWNLOAD, TEXT("Progress:"));
	CProgressCtrl* pWnd = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_DOWNLOAD);
	if (pWnd != NULL)
	{
		pWnd->SetRange(0, 100);
		pWnd->SetStep(1);
	}
	CreateDirectory(m_savePath.c_str(), NULL);

	theApp.m_FloatFrame.ShowWindow(SW_SHOWNORMAL);
	theApp.m_FloatFrame.ShowTopMost();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVFBUPDDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVFBUPDDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVFBUPDDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Automation servers should not exit when a user closes the UI
//  if a controller still holds on to one of its objects.  These
//  message handlers make sure that if the proxy is still in use,
//  then the UI is hidden but the dialog remains around if it
//  is dismissed.

void CVFBUPDDlg::OnClose()
{
	if (CanExit())
		CDialogEx::OnClose();
}

void CVFBUPDDlg::OnOK()
{
	if (m_taskThread == nullptr)
	{
		CString fileUrl = TEXT("");
		GetDlgItemText(IDC_EDIT_URL, fileUrl);
		m_fileUrl = fileUrl;
		if (m_fileUrl.empty())
		{
			return;
		}
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		m_taskThread = std::make_shared<std::thread>([](void* p)
			{
				CVFBUPDDlg* thiz = (CVFBUPDDlg*)p;
				int nRet = 0;
				int nLastPos = 0;
				int nNextPos = 0;
				std::string strResp = ("");
				std::string strFileUrl = ("");
				if (thiz->m_fileUrl.compare(TEXT("http://www.yfvb.com/port.php?c=do&ID=48&a=0")) == 0)
				{
					std::string strVersion = ("");
					std::string strUpdateTime = ("");
					{
						CHttpTool httpTool;
						std::string strSoftWareUrl = ("http://www.yfvb.com/soft-48.htm");
						std::string strStartKey = ("<divalign='center'><imgclass=\"rounded\"src=\"img/softpic/vfb.png\"width='150'height='150'alt=''/>");
						std::string strVersionKeyL = ("<h3>");
						std::string strVersionKeyR = ("</h3>");
						std::string strUpdateTimeKeyL = (">更新日期：");
						std::string strUpdateTimeKeyR = ("&nbsp;&nbsp;软件大小");
						strResp = ("");
						nRet = httpTool.http_get(strSoftWareUrl);
						if (nRet == 0)
						{
							strResp = httpTool.RequestContextPtr()->RespData;
							nLastPos = 0;
							nNextPos = 0;
							nNextPos = strResp.find(strStartKey, nLastPos);
							if (nNextPos != std::string::npos)
							{
								nLastPos = nNextPos + strStartKey.length();
								nNextPos = strResp.find(strVersionKeyL, nLastPos);
								if (nNextPos != std::string::npos)
								{
									nLastPos = nNextPos + strVersionKeyL.length();
									nNextPos = strResp.find(strVersionKeyR, nLastPos);
									if (nNextPos != std::string::npos)
									{
										strVersion = strResp.substr(nLastPos, nNextPos - nLastPos);
										nLastPos = nNextPos + strVersionKeyL.length();
										nNextPos = strResp.find(strUpdateTimeKeyL, nLastPos);
										if (nNextPos != std::string::npos)
										{
											nLastPos = nNextPos + strUpdateTimeKeyL.length();
											nNextPos = strResp.find(strUpdateTimeKeyR, nLastPos);
											if (nNextPos != std::string::npos)
											{
												strUpdateTime = strResp.substr(nLastPos, nNextPos - nLastPos);
											}
										}
									}
								}
							}
						}
					}
					std::string strUpdateUrl = ("");
					{
						CHttpTool httpTool;
						std::string strDownLoadUrl = ("http://www.yfvb.com/port.php?c=do&ID=48&a=0");
						std::string strUpdateUrlKeyL = ("URL=");
						std::string strUpdateUrlKeyR = ("\">");
						nRet = httpTool.http_get(strDownLoadUrl);
						if (nRet == 0)
						{
							strResp = httpTool.RequestContextPtr()->RespData;
							nLastPos = 0;
							nNextPos = 0;
							nNextPos = strResp.find(strUpdateUrlKeyL, nLastPos);
							if (nNextPos != std::string::npos)
							{
								nLastPos = nNextPos + strUpdateUrlKeyL.length();
								nNextPos = strResp.find(strUpdateUrlKeyR, nLastPos);
								if (nNextPos != std::string::npos)
								{
									strUpdateUrl = strResp.substr(nLastPos, nNextPos - nLastPos);
								}
							}
						}
					}
					std::string strUpdate2Url = ("");
					{
						CHttpTool httpTool;
						std::string strStartKey = ("<iframeclass");
						std::string strUpdateUrlKeyL = ("src=\"");
						std::string strUpdateUrlKeyR = ("\"");
						nRet = httpTool.http_get(strUpdateUrl);
						if (nRet == 0)
						{
							strResp = httpTool.GetResult();

							nLastPos = 0;
							nNextPos = 0;
							nNextPos = strResp.find(strUpdateUrlKeyL, nLastPos);
							if (nNextPos != std::string::npos)
							{
								nLastPos = nNextPos + strUpdateUrlKeyL.length();
								nNextPos = strResp.find(strUpdateUrlKeyR, nLastPos);
								if (nNextPos != std::string::npos)
								{
									strUpdate2Url = ("https://yfvb.lanzous.com") + strResp.substr(nLastPos, nNextPos - nLastPos);
								}
							}
						}
					}
					std::string strSignsKey = ("signs");
					std::string strSignsValue = ("");
					std::string strActionKey = ("action");
					std::string strActionValue = ("downprocess");
					std::string strSignKey = ("sign");
					std::string strSignValue = ("");
					std::string strVesKey = ("ves");
					std::string strVesValue = ("");
					std::string strWebSignKey = ("websign");
					std::string strWebSignValue = ("");
					std::string strFilePrefixUrl = ("");
					std::string strFileSuffixUrl = ("");
					{
						//获取POST参数信息
						CHttpTool httpTool;
						std::string strSignsKeyL = ("ajaxdata='");
						std::string strSignsKeyR = ("';");
						std::string strSignKeyL = ("'signs':ajaxdata,'sign':'");
						std::string strSignKeyR = ("','");
						std::string strVesKeyL = ("ves':");
						std::string strVesKeyR = (",'");
						std::string strWebSignKeyL = ("websign':'");
						std::string strWebSignKeyR = ("'");
						nRet = httpTool.http_get(strUpdate2Url);
						if (nRet == 0)
						{
							strResp = httpTool.GetResult();
							nLastPos = 0;
							nNextPos = 0;
							nNextPos = strResp.find(strSignsKeyL, nLastPos);
							if (nNextPos >= 0)
							{
								nLastPos = nNextPos + strSignsKeyL.length();
								nNextPos = strResp.find(strSignsKeyR, nLastPos);
								if (nNextPos >= 0)
								{
									strSignsValue = strResp.substr(nLastPos, nNextPos - nLastPos);
									nLastPos = nNextPos + strSignsKeyR.length();
									nNextPos = strResp.find(strSignKeyL, nLastPos);
									if (nNextPos >= 0)
									{
										nLastPos = nNextPos + strSignKeyL.length();
										nNextPos = strResp.find(strSignKeyR, nLastPos);
										if (nNextPos >= 0)
										{
											strSignValue = strResp.substr(nLastPos, nNextPos - nLastPos);
											nLastPos = nNextPos + strSignKeyR.length();
											nNextPos = strResp.find(strVesKeyL, nLastPos);
											if (nNextPos >= 0)
											{
												nLastPos = nNextPos + strVesKeyL.length();
												nNextPos = strResp.find(strVesKeyR, nLastPos);
												if (nNextPos >= 0)
												{
													strVesValue = strResp.substr(nLastPos, nNextPos - nLastPos);
													nLastPos = nNextPos + strVesKeyR.length();
													nNextPos = strResp.find(strWebSignKeyL, nLastPos);
													if (nNextPos >= 0)
													{
														nLastPos = nNextPos + strWebSignKeyL.length();
														nNextPos = strResp.find(strWebSignKeyR, nLastPos);
														if (nNextPos >= 0)
														{
															strWebSignValue = strResp.substr(nLastPos, nNextPos - nLastPos);
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
					{
						CHttpTool httpTool;
						std::string strPostUrl = ("https://yfvb.lanzous.com/ajaxm.php");
						std::string strFilePrefixUrlKeyL = ("\"dom\":\"");
						std::string strFilePrefixUrlKeyR = ("\",\"");
						std::string strFileSuffixUrlKeyL = ("url\":\"");
						std::string strFileSuffixUrlKeyR = ("\",\"");
						std::string strPostData = ("action=downprocess&signs=") + strSignsValue + ("&sign=") + strSignValue + ("&ves=") + strVesValue + ("&websign=");
						string_replace_all(strPostData, ("\%3F"), ("?"));
						nRet = httpTool.http_post(strPostUrl, strPostData, {
							{TEXT("origin:"),TEXT("https://yfvb.lanzous.com") },
							{TEXT("referer:"),TEXT("https://yfvb.lanzous.com/fn?VzFQOlwyUj4AbgptBWZSa1Q_aUmFRKAVzUGoGMQdtU2MJPFcyC28FZVY2UT9QMw_c_c") },
							});
						if (nRet == 0)
						{
							strResp = httpTool.GetResult();
							nLastPos = 0;
							nNextPos = 0;
							nNextPos = strResp.find(strFilePrefixUrlKeyL, nLastPos);
							if (nNextPos >= 0)
							{
								nLastPos = nNextPos + strFilePrefixUrlKeyL.length();
								nNextPos = strResp.find(strFilePrefixUrlKeyR, nLastPos);
								if (nNextPos >= 0)
								{
									strFilePrefixUrl = strResp.substr(nLastPos, nNextPos - nLastPos);
									nLastPos = nNextPos + strFilePrefixUrlKeyR.length();
									nNextPos = strResp.find(strFileSuffixUrlKeyL, nLastPos);
									if (nNextPos >= 0)
									{
										nLastPos = nNextPos + strFileSuffixUrlKeyL.length();
										nNextPos = strResp.find(strFileSuffixUrlKeyR, nLastPos);
										if (nNextPos >= 0)
										{
											strFileSuffixUrl = strResp.substr(nLastPos, nNextPos - nLastPos);
											if (strFilePrefixUrl.length() > 0 && strFileSuffixUrl.length() > 0)
											{
												string_replace_all(strFilePrefixUrl, (""), ("\\"));
												string_replace_all(strFilePrefixUrl, ("http"), ("https"));
												string_replace_all(strFileSuffixUrl, (""), ("\\"));
												strFileUrl = strFilePrefixUrl + ("/file/") + strFileSuffixUrl;
											}
										}
									}
								}
							}
						}
					}
					{
						CHttpTool httpTool;
						nRet = httpTool.http_get(strFileUrl, true);
						if (nRet == 0)
						{
							strFileUrl = strResp = httpTool.GetResult();
						}
					}
				}
				else 
				{
					strFileUrl = TToA(thiz->m_fileUrl);
				}
				if (!strFileUrl.empty())
				{
					{
						//处理github的zip下载
						std::string githubKeyL = "https://github.com/";
						std::string githubKeyR = "/archive/master.zip";
						nLastPos = 0;
						nNextPos = 0;
						nNextPos = strFileUrl.find(githubKeyL);
						if (nNextPos != std::string::npos)
						{
							nLastPos = nNextPos + githubKeyL.length();
							nNextPos = strFileUrl.find(githubKeyR);
							if (nNextPos != std::string::npos)
							{
								strFileUrl = "https://codeload.github.com/" + strFileUrl.substr(nLastPos, nNextPos - nLastPos) + "/zip/master";
							}
						}
					}
					{
						CHttpTool httpTool;
						nRet = httpTool.http_get_file(TToA(thiz->m_savePath) + ("aaa.7z"), strFileUrl, thiz);
					}
				}
				thiz->GetDlgItem(IDOK)->EnableWindow(TRUE);
				thiz->m_taskThread = NULL;
			}, this);
			m_taskThread->detach();
	}
	//if (CanExit())
	//	CDialogEx::OnOK();
}

void CVFBUPDDlg::OnCancel()
{
	if (CanExit())
		CDialogEx::OnCancel();
}

void CVFBUPDDlg::OnProgress(DWORD dwTotalBytes, DWORD dwExistBytes)
{
	CProgressCtrl* pWnd = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_DOWNLOAD);
	if (pWnd != NULL)
	{
		if (dwTotalBytes == 0)
		{
			dwTotalBytes = dwExistBytes;
		}
		pWnd->SetPos(((float)dwExistBytes / dwTotalBytes) * 100);
		TSTRING tsText = TEXT("Progress: ") + TO_TSTRING(dwExistBytes) + TEXT("*100/") + TO_TSTRING(dwTotalBytes) + TEXT("=") + TO_TSTRING(((float)dwExistBytes / dwTotalBytes) * 100) + TEXT("%");
		SetDlgItemText(IDC_STATIC_DOWNLOAD, tsText.c_str());
	}
}

BOOL CVFBUPDDlg::CanExit()
{
	// If the proxy object is still around, then the automation
	//  controller is still holding on to this application.  Leave
	//  the dialog around, but hide its UI.
	if (m_pAutoProxy != nullptr)
	{
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}

void CVFBUPDDlg::OnBnClickedButtonShow()
{
	// TODO: Add your control notification handler code here
	ShellExecute(NULL, TEXT("OPEN"), TEXT("EXPLORER.EXE"), m_savePath.c_str(), NULL, SW_SHOWNORMAL);
}
