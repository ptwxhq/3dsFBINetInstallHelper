#pragma once
#include "afxcmn.h"
#include "NetInstall.h"
class CFileList :
	public CListCtrl
{
public:
	struct ListData {
		CStringW Stats;
		CStringW Progress;
	};
	CFileList();
	~CFileList();
	void UpdateData(const CNetInstall::tagFileInfo *pinfo, ListData data);
	void AddFile(const CNetInstall::tagFileInfo *pinfo);
	void GetFileLists(std::vector<CNetInstall::tagFileInfo> *pVecInfo);
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDropFiles(HDROP hDropInfo);

protected:
	DWORD m_Innerid;
	std::vector<CNetInstall::tagFileInfo> m_VecInfo;
};

