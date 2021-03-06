
// CiaInstallDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "FileList.h"
#include "NetInstall.h"


// CCiaInstallDlg 对话框
class CCiaInstallDlg : public CDialog
{
// 构造
public:
	CCiaInstallDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CIAINSTALL_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	static UINT WINAPI InstallThd(PVOID parm);
public:
	afx_msg void OnBnClickedConn();
	static int CiaProgress(const CNetInstall::tagFileInfo* pinfo, UINT64 uNowPos);
protected:
	static CFileList m_cialists;
	static HANDLE m_hSendThd;
public:
	afx_msg void OnBnClickedAddfiles();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnEnKillfocusBuffer();
	afx_msg void OnBnClickedClearlist();
	void SetStatus(BOOL bUploading = TRUE);
};
