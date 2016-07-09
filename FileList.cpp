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
	*pVecInfo = m_VecInfo;
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
	m_VecInfo.push_back(info);
	++m_Innerid;
}
