#pragma once
#include "afxcmn.h"
#include "NetInstall.h"
#include <map>
class CFileList :
	public CListCtrl
{
public:
	enum {
		E_SELECT,
		E_ID,
		E_STATUS,
		E_PROGRESS,
		E_ELAPSE,
		E_SPEED,
		E_SIZE,
		E_PATH,
		E_END
	};
	struct ListData {
		CStringW Stats;
		CStringW Progress;
	};
	CFileList();
	~CFileList();
	void UpdateData(const CNetInstall::tagFileInfo *pinfo);
	void AddFile(const CNetInstall::tagFileInfo *pinfo);
	void GetFileLists(std::vector<CNetInstall::tagFileInfo> *pVecInfo);
	DWORD GetCheckCount();
	void ClearList();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDropFiles(HDROP hDropInfo);

protected:
	DWORD m_Innerid;
	std::map<DWORD, CNetInstall::tagFileInfo> m_mapInfo;
public:
	afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
};

