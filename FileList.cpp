#include "stdafx.h"
#include "FileList.h"


CFileList::CFileList()
{
	m_Innerid = 0;
}


CFileList::~CFileList()
{
}

void CFileList::UpdateData(const CNetInstall::tagFileInfo *pinfo, ListData data)
{
	if (!data.Stats.IsEmpty())
	{
		SetItemText(pinfo->dwItemID, 2, data.Stats);
	}
	if (!data.Progress.IsEmpty())
	{
		SetItemText(pinfo->dwItemID, 2, data.Progress);
	}
}

BEGIN_MESSAGE_MAP(CFileList, CListCtrl)
	ON_WM_DROPFILES()
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CFileList::OnLvnItemchanged)
END_MESSAGE_MAP()


void CFileList::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	DWORD nFileCount = 0;

	CStringW strCount;

	nFileCount = DragQueryFileW(hDropInfo, 0xFFFFFFFF, NULL, 0);

	for (size_t i = 0; i < nFileCount; i++)
	{
		WCHAR strNameBuf[MAX_PATH] = { 0 };
		int NameSize = DragQueryFileW(hDropInfo, i, strNameBuf, MAX_PATH);
		if (PathIsDirectoryW(strNameBuf))
		{
			return;
		}

		CNetInstall::tagFileInfo info;
		if (CNetInstall::CheckFile(strNameBuf, &info))
		{
			AddFile(&info);
		}		
	}

	CListCtrl::OnDropFiles(hDropInfo);
}


void CFileList::GetFileLists(std::vector<CNetInstall::tagFileInfo> *pVecInfo)
{
	pVecInfo->clear();
	std::map<DWORD, CNetInstall::tagFileInfo>::iterator it = m_mapInfo.begin();
	for (; it != m_mapInfo.end(); ++it)
	{		
		pVecInfo->push_back(it->second);
	}
}

DWORD CFileList::GetCheckCount()
{
	size_t nCount = 0;
	std::map<DWORD, CNetInstall::tagFileInfo>::iterator it = m_mapInfo.begin();
	for (; it != m_mapInfo.end(); ++it)
	{
		if (it->second.bSelected)
		{
			++nCount;
		}	
	}

	return nCount;
}

void CFileList::ClearList()
{
	m_mapInfo.clear();
	DeleteAllItems();
}

void CFileList::AddFile(const CNetInstall::tagFileInfo *pinfo)
{
	CStringW strTmp;
	size_t nItem = GetItemCount();
	strTmp.Format(L"%d", m_Innerid);
	InsertItem(nItem, L"");
	SetItemText(nItem, 1, strTmp);
	SetItemText(nItem, 2, L"Pending");
	double uSizeUnit = (double)pinfo->uFileSize / 1024;
	if (uSizeUnit > 1024 * 1024)
	{
		strTmp.Format(L"%.2f GB", (double)uSizeUnit / (1024 * 1024) + 0.005);
	}
	else if (uSizeUnit > 1024)
	{
		strTmp.Format(L"%.2f MB", (double)uSizeUnit / 1024 + 0.005);
	}
	else
	{
		strTmp.Format(L"%.2f KB", uSizeUnit + 0.005);
	}
	SetItemText(nItem, 3, strTmp);
	SetItemText(nItem, 4, pinfo->strPath);
	SetCheck(nItem);
	CNetInstall::tagFileInfo info = *pinfo;
	info.dwItemID = m_Innerid;
	m_mapInfo[m_Innerid] = info;
	SetItemData(nItem, m_Innerid);
	++m_Innerid;
}


void CFileList::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	
	if (pNMLV->iItem != -1)
	{
		DWORD dwID = GetItemData(pNMLV->iItem);
		std::map<DWORD, CNetInstall::tagFileInfo>::iterator it = m_mapInfo.find(dwID);
	
		if ((pNMLV->uOldState & INDEXTOSTATEIMAGEMASK(1)) /* old state : unchecked */
			&& (pNMLV->uNewState & INDEXTOSTATEIMAGEMASK(2)) /* new state : checked */
			)
		{
			if (it != m_mapInfo.end())
			{
				it->second.bSelected = true;
				it->second.uFileSize;
				TRACE("id:%d,sel:%d\n", pNMLV->iItem, it->second.bSelected);
			}
		}
		else if ((pNMLV->uOldState & INDEXTOSTATEIMAGEMASK(2)) /* old state : checked */
			&& (pNMLV->uNewState & INDEXTOSTATEIMAGEMASK(1)) /* new state : unchecked */
			)
		{
			if (it != m_mapInfo.end())
			{
				it->second.bSelected = false;
				it->second.uFileSize;
				TRACE("id:%d,sel:%d\n", pNMLV->iItem, it->second.bSelected);
			}
		}				
	}	

	*pResult = 0;
}
