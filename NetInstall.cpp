#include "stdafx.h"
#include "NetInstall.h"
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#pragma comment(lib,"Iphlpapi.lib") //需要添加Iphlpapi.lib库

CNetInstall::UNITPROGRESS CNetInstall::m_sProgressCallBack;

BOOL CNetInstall::SetBufferSize(DWORD dwSize)
{
	if (dwSize < 1024 || dwSize > 1024 * 1024 * 32)
	{
		return FALSE;
	}
	m_dwBufferSize = dwSize;
	return TRUE;
}

DWORD CNetInstall::GetLocalIP()
{
	DWORD dwIP = 0;
	WSAData wsaData;
	if (!WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		char host_name[255];
		if (gethostname(host_name, sizeof(host_name)) != SOCKET_ERROR) {
			struct addrinfo *result = NULL;
			struct addrinfo *ptr = NULL;
			struct addrinfo hints;
			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			getaddrinfo(host_name, 0, &hints, &result);
			if (result)
			{
				for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
					if (ptr->ai_family == AF_INET)
					{
						IN_ADDR *qip = &((struct sockaddr_in *)ptr->ai_addr)->sin_addr;
						dwIP = MAKEIPADDRESS(qip->S_un.S_un_b.s_b1, qip->S_un.S_un_b.s_b2, qip->S_un.S_un_b.s_b3, qip->S_un.S_un_b.s_b4);
						//dwIP = ((struct sockaddr_in *)ptr->ai_addr)->sin_addr.S_un.S_addr;
						if (dwIP)
						{
							break;
						}
						
					}
				}
				
			}
		}	
	}
	
	WSACleanup();
	return dwIP;
}

BOOL CNetInstall::Connect(LPCWSTR lpszAddr, WORD wPort/* = 5000*/)
{
	if (!m_bInited)
	{
		WSAData wd;
		int ret = WSAStartup(MAKEWORD(2, 2), &wd);
		if (!ret)
		{
			m_bInited = true;
		}
	}

	if (!m_socket)
	{
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	}
	sockaddr_in ser_addr;
	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	InetPtonW(AF_INET, lpszAddr, &ser_addr.sin_addr.s_addr);
	ser_addr.sin_port = htons(wPort);
	return !connect(m_socket, (sockaddr *)&ser_addr, sizeof(sockaddr));
}

BOOL CNetInstall::StopConn()
{
	BOOL bRet = FALSE;
	bRet = !shutdown(m_socket, SD_BOTH);
	if (bRet)
	{
		DisConnect();
	}
	return bRet;
}

BOOL CNetInstall::SendData(LPBYTE lpData, DWORD dwLen)
{
	int res = 0;
	int ret;

	while (1)
	{
		TRACE(L"SendData!\n");
		ret = send(m_socket, (CHAR*)lpData + res, dwLen, 0);
		if (-1 == ret)
		{
			return FALSE;
		}
		else if (ret > 0)
		{
			res += ret;
			dwLen -= ret;
			if (0 == dwLen)
			{
				return res;
			}
		}
		else if (0 == ret)
		{
			return res;
		}
	}

}

BOOL CNetInstall::RecvData(LPBYTE lpData, DWORD dwLen)
{
	int res = 0;
	int ret;

	while (1)
	{
		TRACE(L"RecvData!\n");
		ret = recv(m_socket, (CHAR*)lpData + res, dwLen, 0);
		if (-1 == ret)
		{
			return FALSE;
		}
		else if (0 == ret)
		{
			return res;
		}

		res += ret;
		dwLen -= ret;
		if (0 == dwLen)
		{
			return res;
		}
	}
}

BOOL CNetInstall::DisConnect()
{
	if (m_socket)
	{
		if (!closesocket(m_socket))
		{
			m_socket = 0;
			return TRUE;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CNetInstall::TransFile(const tagFileInfo* pInfo)
{
	BOOL bRet = FALSE;
	HANDLE hFile = CreateFileW(pInfo->strPath, FILE_GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwRead = 0;
		int iErrorCount = 0;
		bool bSendErr = false;
		PBYTE pBuf = new BYTE[m_dwBufferSize];
		UINT64 uLeftSize = pInfo->uFileSize;
		while (uLeftSize > 0)
		{	
			if (bSendErr)
			{
				if (SendData(pBuf, dwRead))
				{
					bSendErr = false;
					uLeftSize -= dwRead;
					iErrorCount = 0;
				}
				else
				{
					++iErrorCount;
					if (iErrorCount > 3)
					{
						break;
					}
				}
			}
			else if (ReadFile(hFile, pBuf, m_dwBufferSize, &dwRead, 0))
			{
				if (SendData(pBuf, dwRead))
				{
					uLeftSize -= dwRead;
				}
				else
				{
					bSendErr = true;
				}
				
			}
			else
			{
				++iErrorCount;
				if (iErrorCount > 2)
				{
					break;
				}
			}

			if (!m_sProgressCallBack(pInfo, pInfo->uFileSize - uLeftSize))
			{
				StopConn();
				break;
			}
		}
		delete [] pBuf;

		if (uLeftSize == 0)
		{
			bRet = TRUE;
		}
	}


	return bRet;
}


BOOL CNetInstall::CheckFile(LPCWSTR lpszPath, tagFileInfo* pInfo)
{
	BOOL bRet = FALSE;
	if (StrCmpIW(PathFindExtensionW(lpszPath), L".cia"))
	{
		return FALSE;
	}
	HANDLE hFile = CreateFileW(lpszPath, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER LargeInt;
		if (GetFileSizeEx(hFile, &LargeInt))
		{
			if (pInfo)
			{
				pInfo->uFileSize = LargeInt.QuadPart;
				pInfo->strPath = lpszPath;
				bRet = TRUE;
			}
		}
		CloseHandle(hFile);		
	}
	return bRet;
}

void CNetInstall::StartTask(const std::vector<tagFileInfo> *pVecInfo)
{
	size_t nCount = pVecInfo->size();
	size_t nRealCount = 0;
	for (size_t i = 0; i < nCount; ++i)
	{
		if ((*pVecInfo)[i].bSelected)
		{
			++nRealCount;
		}
	}

	UINT32 nTs = htonl(nRealCount);
	if (SendData((LPBYTE)&nTs, sizeof(nTs)))
	{
		UINT8 ack = 0;
		RecvData(&ack, sizeof(ack));
		for (size_t i = 0; i < nCount; ++i)
		{
			m_sProgressCallBack(&(*pVecInfo)[i], 0);
			if (!(*pVecInfo)[i].bSelected)		
				continue;
				
			UINT64 SendNetNum = htonll((*pVecInfo)[i].uFileSize);
			if (SendData((LPBYTE)&SendNetNum, sizeof(SendNetNum)))
			{
				if (!TransFile(&(*pVecInfo)[i]))
				{
					break;
				}

			}
			else
			{
				break;
				//((*pVecInfo)[i]).dwError = 1;
				//m_ProgressCallBack(i, &(*pVecInfo)[i], 0);
				OutputDebugStringA("err fhapp");
			}
		}
	}
	else
	{
		MessageBoxW(0, L"eeeerr", 0, 0);
	}

	DisConnect();
}

CNetInstall::CNetInstall()
{
	m_bInited = false;
	m_socket = NULL;
	m_dwBufferSize = 256 * 1024;
}


CNetInstall::~CNetInstall()
{
	DisConnect();
	if (m_bInited)
	{
		WSACleanup();
	}
}

