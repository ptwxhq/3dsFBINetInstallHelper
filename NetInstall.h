#pragma once
#include <vector>


class CNetInstall
{
public:
	enum {
		ERR_NOSET,
		ERR_FAIL,
	};

	struct tagFileInfo {
		bool bSelected;
		DWORD dwItemID;
		DWORD dwError;
		DWORD dwBeginTime;
//		DWORD dwStopTime;
		UINT64 uFileSize;
		UINT64 uSendSize;
		CStringW strPath;
		tagFileInfo() {
			bSelected = true;
			uFileSize = 0;
			uSendSize = 0;
			dwError = 0;
		}
	};
	typedef int(*UNITPROGRESS)(const tagFileInfo*, UINT64 uNowPos);

	BOOL SetBufferSize(DWORD dwSize);
	static DWORD GetLocalIP();
	BOOL Connect(LPCWSTR lpszAddr, WORD wVerType = MAKEWORD(2, 0), WORD wPort = 5000);
	BOOL StopConn();
	BOOL DisConnect();
	static BOOL CheckFile(LPCWSTR lpszPath, tagFileInfo* pInfo = NULL);
	void StartTask(std::vector<tagFileInfo> *pVecInfo);
	BOOL TransFile(tagFileInfo* pInfo);
	
	CNetInstall();
	~CNetInstall();
	static void SetCallBack(UNITPROGRESS cb) {
		m_sProgressCallBack = cb;
	}
protected:
	BOOL SendData(LPBYTE lpData, DWORD dwLen);
	BOOL RecvData(LPBYTE lpData, DWORD dwLen);
	bool m_bInited;
	WORD m_wFBIVer;
	DWORD m_dwBufferSize;
	SOCKET m_socket;
	static UNITPROGRESS m_sProgressCallBack;
};

