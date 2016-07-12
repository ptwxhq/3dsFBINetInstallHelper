#include "stdafx.h"
#include "FileList.h"


CFileList::CFileList()
{
	m_Innerid = 0;
}


CFileList::~CFileList()
{
}

void CFileList::UpdateData(const CNetInstall::tagFileInfo *pinfo)
{
	CStringW strStatus;
	CStringW strProgress;
	if (CNetInstall::ERR_FAIL == pinfo->dwError)
	{
		strStatus = L"Failed";
	}
	else if (pinfo->uSendSize == 0)
	{
		if (pinfo->bSelected)
		{
			strStatus = L"Uploading";
		}
		else
		{
			strStatus = L"Skipped";
			strProgress = L"N/A";
		}
		
	}
	else if (pinfo->uSendSize > 0)
	{
		strProgress.Format(L"%.2f%%", (double)pinfo->uSendSize * 100 / pinfo->uFileSize);
	}
	

	if (!strStatus.IsEmpty())
	{
		SetItemText(pinfo->dwItemID, E_STATUS, strStatus);
	}
	if (!strProgress.IsEmpty())
	{
		SetItemText(pinfo->dwItemID, E_PROGRESS, strProgress);
	}

	int iElapse = GetTickCount() - pinfo->dwBeginTime;
	if (iElapse < 0)
	{
		iElapse = MAXDWORD - pinfo->dwBeginTime;
	}
	CStringW strTime;
	DWORD dwTotalSec = iElapse / 1000;
	WORD wHour = dwTotalSec / 3600;
	WORD wMinute = dwTotalSec % 3600 / 60;
	WORD wSecond = dwTotalSec % 60;
	strTime.Format(L"%d:%02d:%02d", wHour, wMinute, wSecond);
	SetItemText(pinfo->dwItemID, E_ELAPSE, strTime);

	if (pinfo->bSelected)
	{
		CStringW strSpeed;
		if (pinfo->uSendSize && dwTotalSec)
		{
			
			double fSpeed = (double)pinfo->uSendSize / dwTotalSec;
			if (fSpeed > 1024 * 1024)
			{
				strSpeed.Format(L"%.1fMB/S", fSpeed / (1024 * 1024));
			}
			else if (fSpeed > 1024)
			{
				strSpeed.Format(L"%.1fKB/S", fSpeed / 1024);
			}
			else
			{
				strSpeed.Format(L"%.1fB/S", fSpeed);
			}
			
		}
		else {
			strSpeed = L"0B/S";
		}
		SetItemText(pinfo->dwItemID, E_SPEED, strSpeed);
		
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
	SetItemText(nItem, E_ID, strTmp);
	SetItemText(nItem, E_STATUS, L"Pending");
	SetItemText(nItem, E_PROGRESS, L"N/A");
	SetItemText(nItem, E_ELAPSE, L"0:00:00");
	SetItemText(nItem, E_SPEED, L"N/A");

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

	SetItemText(nItem, E_SIZE, strTmp);
	SetItemText(nItem, E_PATH, pinfo->strPath);
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
