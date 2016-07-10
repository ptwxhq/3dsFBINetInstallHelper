#pragma once
#include <vector>


class CNetInstall
{
public:
	struct tagFileInfo {
		bool bSelected;
		DWORD dwItemID;
		DWORD dwError;
//		DWORD dwBeginTime;
//		DWORD dwStopTime;
		UINT64 uFileSize;
		CStringW strPath;
		tagFileInfo() {
			bSelected = true;
			uFileSize = 0;
			dwError = 0;
		}
	};
	typedef int(*UNITPROGRESS)(const tagFileInfo*, UINT64 uNowPos);

	BOOL SetBufferSize(DWORD dwSize);
	static DWORD GetLocalIP();
	BOOL Connect(LPCWSTR lpszAddr, WORD wPort = 5000);
	BOOL StopConn();
	BOOL DisConnect();
	static BOOL CheckFile(LPCWSTR lpszPath, tagFileInfo* pInfo = NULL);
	void StartTask(const std::vector<tagFileInfo> *pVecInfo);
	BOOL TransFile(const tagFileInfo* pInfo);
	
	CNetInstall();
	~CNetInstall();
	static void SetCallBack(UNITPROGRESS cb) {
		m_sProgressCallBack = cb;
	}
protected:
	BOOL SendData(LPBYTE lpData, DWORD dwLen);
	BOOL RecvData(LPBYTE lpData, DWORD dwLen);
	bool m_bInited;
	DWORD m_dwBufferSize;
	SOCKET m_socket;
	static UNITPROGRESS m_sProgressCallBack;
};

