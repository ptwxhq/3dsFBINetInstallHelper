
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
	m_cialists.InsertColumn(0, _T("Sel"), LVCFMT_LEFT, 32);
	m_cialists.InsertColumn(2, _T("ID"), LVCFMT_LEFT, 50);
	m_cialists.InsertColumn(5, _T("Stats"), LVCFMT_LEFT, 80);
	m_cialists.InsertColumn(7, _T("Size"), LVCFMT_LEFT, 90);
	m_cialists.InsertColumn(10, _T("Path"), LVCFMT_LEFT, 400);

	CNetInstall::SetCallBack(CiaProgress);


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

int CCiaInstallDlg::CiaProgress(const CNetInstall::tagFileInfo* pinfo, UINT64 uNowPos)
{
	if (!m_hSendThd)
		return 0;

	CFileList::ListData ld;
	if (uNowPos == 0 && !pinfo->bSelected)
	{
		ld.Stats = L"Skipped";
		ld.Progress = L"N/A";
	}
	else if (uNowPos > 0)
	{
		ld.Progress.Format(L"%.2f%%", (double)uNowPos * 100 / pinfo->uFileSize);
	}
	m_cialists.UpdateData(pinfo, ld);
	return 1;
//	pinfo->dwItemID
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
	//GetDlgItemTextW(IDC_IPADDR, ip);
	//GetWindowText()
	BYTE ipb[4] = { 0 };
	((CIPAddressCtrl*)pDlg->GetDlgItem(IDC_IPADDR))->GetAddress(ipb[0], ipb[1], ipb[2], ipb[3]);
	ips.Format(L"%d.%d.%d.%d", ipb[0], ipb[1], ipb[2], ipb[3]);

	if (nis.Connect(ips))
	{
		((CEdit *)FindWindowEx(pDlg->GetDlgItem(IDC_IPADDR)->GetSafeHwnd(), 0, 0, 0))->ShowBalloonTip(L"please wait and press ok on your 3ds", L"conn succ", TTI_INFO);
		DWORD dwIP = 0;
		((CIPAddressCtrl*)pDlg->GetDlgItem(IDC_IPADDR))->GetAddress(dwIP);
		ips.Format(L"%d", dwIP);
		WritePrivateProfileStringW(L"3DS", L"IPV4", ips, _SETTING_FILE_NAME);
		OutputDebugStringW(L"conn succ");
		std::vector<CNetInstall::tagFileInfo> vecInfo;
		pDlg->m_cialists.GetFileLists(&vecInfo);

		pDlg->GetDlgItem(IDC_CONN)->SetWindowText(L"Stop!!");
		pDlg->GetDlgItem(IDC_CONN)->EnableWindow();

		nis.StartTask(&vecInfo);

		pDlg->GetDlgItem(IDC_CONN)->SetWindowText(L"Go!Go!Go!");
		pDlg->GetDlgItem(IDC_CONN)->EnableWindow();
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
