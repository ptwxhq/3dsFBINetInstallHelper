
// CiaInstallDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CiaInstall.h"
#include "CiaInstallDlg.h"
#include "afxdialogex.h"
#include "NetInstall.h"

#define _SETTING_FILE_NAME L".\\settings.ini"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CCiaInstallDlg 对话框

CFileList CCiaInstallDlg::m_cialists;
HANDLE CCiaInstallDlg::m_hSendThd = NULL;


CCiaInstallDlg::CCiaInstallDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_CIAINSTALL_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCiaInstallDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELISTS, m_cialists);
}

BEGIN_MESSAGE_MAP(CCiaInstallDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CONN, &CCiaInstallDlg::OnBnClickedConn)
	ON_BN_CLICKED(IDC_ADDFILES, &CCiaInstallDlg::OnBnClickedAddfiles)
	ON_EN_KILLFOCUS(IDC_BUFFER, &CCiaInstallDlg::OnEnKillfocusBuffer)
	ON_BN_CLICKED(IDC_CLEARLIST, &CCiaInstallDlg::OnBnClickedClearlist)
END_MESSAGE_MAP()


// CCiaInstallDlg 消息处理程序

BOOL CCiaInstallDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	GetDlgItem(IDC_PORT)->SetWindowText(L"5000");

	CComboBox *pVerCB = (CComboBox*)GetDlgItem(IDC_FBIVER);
	pVerCB->SetItemData(0, MAKEWORD(1, 0));
	pVerCB->SetItemData(1, MAKEWORD(2, 0));
	pVerCB->SetCurSel(1);

	DWORD dwIP = GetPrivateProfileIntW(L"3DS", L"IPV4", 0, _SETTING_FILE_NAME);
	if (!dwIP)
	{
		dwIP = CNetInstall::GetLocalIP();
	}
	//Buffer
	if (dwIP)
	{
		((CIPAddressCtrl*)GetDlgItem(IDC_IPADDR))->SetAddress(dwIP);
	}
	
	WCHAR buf[10] = L"256";
	GetPrivateProfileStringW(L"3DS", L"Buffer", L"256", buf, 10, _SETTING_FILE_NAME);
	GetDlgItem(IDC_BUFFER)->SetWindowText(buf);
	((CEdit*)GetDlgItem(IDC_BUFFER))->SetLimitText(5);

	m_cialists.SetExtendedStyle(m_cialists.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES|LVS_EX_INFOTIP);

	WCHAR astrTitleName[][20] = {
		L"Sel", L"ID", L"Status", L"Progress", L"Elapse", L"Speed", L"Size", L"Path"
	};
	DWORD adwMT[] = { LVCFMT_CENTER , LVCFMT_LEFT , LVCFMT_CENTER , LVCFMT_CENTER , LVCFMT_CENTER , LVCFMT_CENTER , LVCFMT_CENTER , LVCFMT_LEFT };

	WORD awTitleLen[] = { 32 , 30 , 80, 70 , 80 , 90 , 90 , 400 };

	for (size_t i = CFileList::E_SELECT; i < CFileList::E_END; ++i)
	{
		m_cialists.InsertColumn(CFileList::E_PATH, astrTitleName[i], adwMT[i], awTitleLen[i]);
	}


	CNetInstall::SetCallBack(CiaProgress);


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

int CCiaInstallDlg::CiaProgress(const CNetInstall::tagFileInfo* pinfo, UINT64 uNowPos)
{
	if (!m_hSendThd)
		return 0;

	m_cialists.UpdateData(pinfo);
	return 1;
}

void CCiaInstallDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CCiaInstallDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCiaInstallDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//CNetInstall nis;
CToolTipCtrl tp;
CToolTipCtrl *m_pToolTipCtrl;


UINT WINAPI CCiaInstallDlg::InstallThd(PVOID parm)
{
	CCiaInstallDlg *pDlg = (CCiaInstallDlg*)parm;
	CNetInstall nis;
	CString strTmp;
	pDlg->GetDlgItem(IDC_BUFFER)->GetWindowText(strTmp);
	nis.SetBufferSize(_wtoi(strTmp));
	WritePrivateProfileStringW(L"3DS", L"Buffer", strTmp, _SETTING_FILE_NAME);
	CString ips;
	WORD wVer = 2, wPort = 5000;
	pDlg->GetDlgItemText(IDC_PORT, strTmp);
	if (!strTmp.IsEmpty())
	{
		wPort = _wtoi(strTmp);
	}
	int iSel = ((CComboBox*)pDlg->GetDlgItem(IDC_FBIVER))->GetCurSel();
	wVer = (WORD)((CComboBox*)pDlg->GetDlgItem(IDC_FBIVER))->GetItemData(iSel);
	switch (wVer)
	{
	case MAKEWORD(1, 0):
	case MAKEWORD(2, 0):
		break;
	default:
		wVer = MAKEWORD(2, 0);
		break;
	}
	
	BYTE ipb[4] = { 0 };
	((CIPAddressCtrl*)pDlg->GetDlgItem(IDC_IPADDR))->GetAddress(ipb[0], ipb[1], ipb[2], ipb[3]);
	ips.Format(L"%d.%d.%d.%d", ipb[0], ipb[1], ipb[2], ipb[3]);

	if (nis.Connect(ips, wVer))
	{
		((CEdit *)FindWindowEx(pDlg->GetDlgItem(IDC_IPADDR)->GetSafeHwnd(), 0, 0, 0))->ShowBalloonTip(L"please wait and press ok on your 3ds", L"conn succ", TTI_INFO);
		DWORD dwIP = 0;
		((CIPAddressCtrl*)pDlg->GetDlgItem(IDC_IPADDR))->GetAddress(dwIP);
		ips.Format(L"%d", dwIP);
		WritePrivateProfileStringW(L"3DS", L"IPV4", ips, _SETTING_FILE_NAME);
		OutputDebugStringW(L"conn succ");
		std::vector<CNetInstall::tagFileInfo> vecInfo;
		pDlg->m_cialists.GetFileLists(&vecInfo);

		pDlg->SetStatus();

		nis.StartTask(&vecInfo);

		pDlg->SetStatus(FALSE);
	}
	else
	{
		
		((CEdit *)FindWindowEx(pDlg->GetDlgItem(IDC_IPADDR)->GetSafeHwnd(), 0, 0, 0))->ShowBalloonTip(L"check your network or ip you input", L"conn failed", TTI_ERROR);
		pDlg->GetDlgItem(IDC_CONN)->EnableWindow();
	}

	return 0;
}



void CCiaInstallDlg::OnBnClickedConn()
{
	// TODO: 在此添加控件通知处理程序代码
	//STILL_ACTIVE
	if (m_hSendThd)
	{
		DWORD dwRet = WaitForSingleObject(m_hSendThd, 0);
		if (WAIT_TIMEOUT == dwRet)
		{
			GetDlgItem(IDC_CONN)->EnableWindow(FALSE);
			OutputDebugStringA("Thd OK");
			CloseHandle(m_hSendThd);
			m_hSendThd = NULL;
			return;
		}
		else
		{
			m_hSendThd = NULL;
		}
	}

	if (m_cialists.GetCheckCount() < 1)
	{
		((CEdit *)FindWindowEx(GetDlgItem(IDC_IPADDR)->GetSafeHwnd(), 0, 0, 0))->ShowBalloonTip(L"no file now", L"nothing exist", TTI_ERROR);
		return;
	}

	if (!((CIPAddressCtrl*)GetDlgItem(IDC_IPADDR))->IsBlank())
	{
		GetDlgItem(IDC_CONN)->EnableWindow(FALSE);
		m_hSendThd = (HANDLE)_beginthreadex(0, 0, InstallThd, this, 0, 0);
		//CloseHandle(m_hSendThd);
	}
	else
	{
		((CEdit *)FindWindowEx(GetDlgItem(IDC_IPADDR)->GetSafeHwnd(), 0, 0, 0))->ShowBalloonTip(L"check the ip you input", L"failed", TTI_ERROR);
	}
	
}

void CCiaInstallDlg::OnBnClickedAddfiles()
{
	// TODO: 在此添加控件通知处理程序代码
#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

	CStringW fileName;
	wchar_t* p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
	CFileDialog fd(TRUE);
	OPENFILENAMEW& ofn = fd.GetOFN();
	ofn.Flags |= OFN_ALLOWMULTISELECT;
	ofn.lpstrFile = p;
	ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;
	ofn.lpstrDefExt = L".cia";
	ofn.lpstrFilter = L"3ds cia files (*.cia)\0*.cia\0\0";
	fd.DoModal();
	fileName.ReleaseBuffer();
	wchar_t* pBufEnd = p + FILE_LIST_BUFFER_SIZE - 2;
	wchar_t* start = p;
	while ((p < pBufEnd) && (*p))
		p++;
	if (p > start)
	{
		CStringW strFolder = start;
		if (PathIsDirectoryW(strFolder))
		{
			strFolder += L"\\";
		}
		else
		{
			CNetInstall::tagFileInfo info;
			if (CNetInstall::CheckFile(strFolder, &info))
			{
				m_cialists.AddFile(&info);
			}
		
			return;
		}
		
		p++;

		int fileCount = 1;
		while ((p < pBufEnd) && (*p))
		{
			start = p;
			while ((p < pBufEnd) && (*p))
				p++;
			if (p > start)
			{
				CNetInstall::tagFileInfo info;
				if (CNetInstall::CheckFile(strFolder + start, &info))
				{
					m_cialists.AddFile(&info);
				}		
			}
				
			p++;
			fileCount++;
		}
	}

}


BOOL CCiaInstallDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	if (NULL != m_pToolTipCtrl)
		m_pToolTipCtrl->RelayEvent(pMsg);


	return CDialog::PreTranslateMessage(pMsg);
}


void CCiaInstallDlg::OnEnKillfocusBuffer()
{
	CString value;
	GetDlgItem(IDC_BUFFER)->GetWindowText(value);
	DWORD dwV = _wtoi(value);
	if (dwV < 1 || dwV>32 * 1024)
	{
		GetDlgItem(IDC_BUFFER)->SetWindowText(L"256");
	}
	// TODO: 在此添加控件通知处理程序代码
}


void CCiaInstallDlg::OnBnClickedClearlist()
{
	// TODO: 在此添加控件通知处理程序代码
	m_cialists.ClearList();
}

void CCiaInstallDlg::SetStatus(BOOL bUploading /*= TRUE*/)
{
	LONG lFLExStyle = GetWindowLongW(GetDlgItem(IDC_FILELISTS)->GetSafeHwnd(), GWL_EXSTYLE);
	if (bUploading)
	{
		GetDlgItem(IDC_CONN)->SetWindowText(L"Stop!!");
		GetDlgItem(IDC_CONN)->EnableWindow();

		GetDlgItem(IDC_ADDFILES)->EnableWindow(FALSE);
		GetDlgItem(IDC_CLEARLIST)->EnableWindow(FALSE);
	
		SetWindowLong(GetDlgItem(IDC_FILELISTS)->GetSafeHwnd(), GWL_EXSTYLE, lFLExStyle & ~lFLExStyle);
	}
	else
	{
		GetDlgItem(IDC_CONN)->SetWindowText(L"Go!Go!Go!");
		GetDlgItem(IDC_CONN)->EnableWindow();

		GetDlgItem(IDC_ADDFILES)->EnableWindow();
		GetDlgItem(IDC_CLEARLIST)->EnableWindow();
		GetDlgItem(IDC_FILELISTS)->EnableWindow();

		SetWindowLong(GetDlgItem(IDC_FILELISTS)->GetSafeHwnd(), GWL_EXSTYLE, lFLExStyle | WS_EX_ACCEPTFILES);
	}
}
