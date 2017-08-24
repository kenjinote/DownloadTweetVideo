#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "Rpcrt4")
#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "wininet")

#include <windows.h>
#include <Shlwapi.h>
#include <wininet.h>
#include <vector>
#include <string>
#include "resource.h"

TCHAR szClassName[] = TEXT("Window");

LPBYTE DownloadToMemory(IN LPCWSTR lpszURL)
{
	LPBYTE lpszReturn = 0;
	DWORD dwSize = 0;
	const HINTERNET hSession = InternetOpenW(L"Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/38.0.2125.111 Safari/537.36", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, INTERNET_FLAG_NO_COOKIES);
	if (hSession)
	{
		URL_COMPONENTSW uc = { 0 };
		WCHAR HostName[MAX_PATH];
		WCHAR UrlPath[MAX_PATH];
		uc.dwStructSize = sizeof(uc);
		uc.lpszHostName = HostName;
		uc.lpszUrlPath = UrlPath;
		uc.dwHostNameLength = MAX_PATH;
		uc.dwUrlPathLength = MAX_PATH;
		InternetCrackUrlW(lpszURL, 0, 0, &uc);
		const HINTERNET hConnection = InternetConnectW(hSession, HostName, INTERNET_DEFAULT_HTTP_PORT, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
		if (hConnection)
		{
			const HINTERNET hRequest = HttpOpenRequestW(hConnection, L"GET", UrlPath, 0, 0, 0, 0, 0);
			if (hRequest)
			{
				ZeroMemory(&uc, sizeof(URL_COMPONENTS));
				WCHAR Scheme[16];
				uc.dwStructSize = sizeof(uc);
				uc.lpszScheme = Scheme;
				uc.lpszHostName = HostName;
				uc.dwSchemeLength = 16;
				uc.dwHostNameLength = MAX_PATH;
				InternetCrackUrlW(lpszURL, 0, 0, &uc);
				WCHAR szReferer[1024];
				lstrcpyW(szReferer, L"Referer: ");
				lstrcatW(szReferer, Scheme);
				lstrcatW(szReferer, L"://");
				lstrcatW(szReferer, HostName);
				HttpAddRequestHeadersW(hRequest, szReferer, lstrlenW(szReferer), HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
				HttpSendRequestW(hRequest, 0, 0, 0, 0);
				lpszReturn = (LPBYTE)GlobalAlloc(GMEM_FIXED, 1);
				DWORD dwRead;
				BYTE szBuf[1024 * 4];
				LPBYTE lpTmp;
				for (;;)
				{
					if (!InternetReadFile(hRequest, szBuf, (DWORD)sizeof(szBuf), &dwRead) || !dwRead) break;
					lpTmp = (LPBYTE)GlobalReAlloc(lpszReturn, (SIZE_T)(dwSize + dwRead), GMEM_MOVEABLE);
					if (lpTmp == NULL) break;
					lpszReturn = lpTmp;
					CopyMemory(lpszReturn + dwSize, szBuf, dwRead);
					dwSize += dwRead;
				}
				InternetCloseHandle(hRequest);
			}
			InternetCloseHandle(hConnection);
		}
		InternetCloseHandle(hSession);
	}
	return lpszReturn;
}

BOOL DownloadToFile(IN LPCWSTR lpszURL, IN LPCWSTR lpszFilePath)
{
	BOOL bReturn = FALSE;
	LPBYTE lpByte = DownloadToMemory(lpszURL);
	if (lpByte)
	{
		const DWORD nSize = (DWORD)GlobalSize(lpByte);
		const HANDLE hFile = CreateFileW(lpszFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwWritten;
			WriteFile(hFile, lpByte, nSize, &dwWritten, NULL);
			CloseHandle(hFile);
			bReturn = TRUE;
		}
		GlobalFree(lpByte);
	}
	return bReturn;
}

BOOL CreateTempDirectory(LPWSTR pszDir)
{
	DWORD dwSize = GetTempPath(0, 0);
	if (dwSize == 0 || dwSize > MAX_PATH - 14) { goto END0; }
	LPWSTR pTmpPath;
	pTmpPath = (LPWSTR)GlobalAlloc(GPTR, sizeof(WCHAR)*(dwSize + 1));
	GetTempPathW(dwSize + 1, pTmpPath);
	dwSize = GetTempFileNameW(pTmpPath, L"", 0, pszDir);
	GlobalFree(pTmpPath);
	if (dwSize == 0) { goto END0; }
	DeleteFileW(pszDir);
	if (CreateDirectoryW(pszDir, 0) == 0) { goto END0; }
	return TRUE;
END0:
	return FALSE;
}

VOID MyCreateFileFromResource(WCHAR *szResourceName, WCHAR *szResourceType, WCHAR *szResFileName)
{
	DWORD dwWritten;
	HRSRC hRs = FindResourceW(0, szResourceName, szResourceType);
	DWORD dwResSize = SizeofResource(0, hRs);
	HGLOBAL hMem = LoadResource(0, hRs);
	LPBYTE lpByte = (BYTE *)LockResource(hMem);
	HANDLE hFile = CreateFileW(szResFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, lpByte, dwResSize, &dwWritten, NULL);
	CloseHandle(hFile);
}

std::wstring Replace(std::wstring String1, std::wstring String2, std::wstring String3)
{
	std::wstring::size_type Pos(String1.find(String2));
	while (Pos != std::wstring::npos)
	{
		String1.replace(Pos, String2.length(), String3);
		Pos = String1.find(String2, Pos + String3.length());
	}
	return String1;
}

LPWSTR Download2WChar(LPCWSTR lpszTweetURL)
{
	LPWSTR lpszReturn = 0;
	LPBYTE lpByte = DownloadToMemory(lpszTweetURL);
	if (lpByte)
	{
		const DWORD len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)lpByte, -1, 0, 0);
		lpszReturn = (LPWSTR)GlobalAlloc(0, (len + 1) * sizeof(WCHAR));
		if (lpszReturn)
		{
			MultiByteToWideChar(CP_UTF8, 0, (LPCCH)lpByte, -1, lpszReturn, len);
		}
		GlobalFree(lpByte);
	}
	return lpszReturn;
}

LPCWSTR GetFileName(LPCWSTR lpszURL)
{
	LPCWSTR p = lpszURL;
	while (*lpszURL != L'\0')
	{
		if (*lpszURL == L'/')
			p = lpszURL + 1;
		++lpszURL;
	}
	return p;
}

BOOL GetRedirectURL(LPCWSTR lpszSourceURL, LPWSTR lpszRedirectURL)
{
	BOOL bReturn = FALSE;
	LPWSTR lpszWeb = Download2WChar(lpszSourceURL);
	if (!lpszWeb) return 0;
	std::wstring srcW(lpszWeb);
	GlobalFree(lpszWeb);
	size_t posStart = srcW.find(L"<META http-equiv=\"refresh\" content=\"0;URL=");
	if (posStart != std::wstring::npos)
	{
		posStart += 42;
		size_t posEnd = srcW.find(L'\"', posStart);
		if (posEnd != std::wstring::npos)
		{
			std::wstring strRedirectURL(srcW, posStart, posEnd - posStart);
			lstrcpyW(lpszRedirectURL, strRedirectURL.c_str());
			bReturn = TRUE;
		}
	}
	return bReturn;
}

BOOL DownloadTwitterVideo(LPCWSTR lpszTweetURL, LPCWSTR lpszOutputFolder = 0)
{
	BOOL bReturnValue = FALSE;
	WCHAR szURL[1024] = { 0 };
	WCHAR szUserName[256] = { 0 };
	WCHAR szTweetID[256] = { 0 };
	if (wcsstr(lpszTweetURL, L"//t.co/"))
	{
		GetRedirectURL(lpszTweetURL, szURL);
	}
	else
	{
		lstrcat(szURL, lpszTweetURL);
	}
	{
		std::wstring url(szURL);
		size_t posStart = url.find(L"//twitter.com/"); //"https://twitter.com/SimonsCat/status/816626503106445313"
		if (posStart == std::wstring::npos) return bReturnValue;
		posStart += 14;
		size_t posEnd;
		posEnd = url.find(L'/', posStart);
		if (posEnd == std::wstring::npos) return bReturnValue;
		std::wstring strUserName(url, posStart, posEnd - posStart);
		lstrcpyW(szUserName, strUserName.c_str());
		posStart = url.find(L"status/", posEnd);
		if (posStart == std::wstring::npos) return bReturnValue;
		posStart += 7;
		posEnd = url.find(L'/', posStart);
		if (posEnd == std::wstring::npos)
		{
			std::wstring strTweetID(url, posStart);
			lstrcpyW(szTweetID, strTweetID.c_str());
		}
		else
		{
			std::wstring strTweetID(url, posStart, posEnd - posStart);
			lstrcpyW(szTweetID, strTweetID.c_str());
		}
		if (szUserName[0] == 0 || szTweetID[0] == 0)
		{
			return bReturnValue;
		}
	}
	LPWSTR lpszWeb = Download2WChar(szURL);
	if (!lpszWeb) return 0;
	std::wstring srcW(lpszWeb);
	GlobalFree(lpszWeb);
	size_t posStart = srcW.find(L"<meta  property=\"og:video:url\" content=\"");
	size_t posEnd;
	if (posStart == std::wstring::npos)
	{
		//見つからない場合は、アニメーションGIFとみなして動画(mp4)のURLを探ってみる
		posStart = srcW.find(L"PlayableMedia--gif");
		if (posStart == std::wstring::npos) return 0;
		posStart += 18;
		posStart = srcW.find(L"background-image:url(", posStart);
		if (posStart == std::wstring::npos) return 0;
		posStart += 21;
		posEnd = srcW.find(L')', posStart);
		std::wstring url(srcW, posStart + 1, posEnd - posStart - 2);
		posStart = url.rfind(L'/');
		++posStart;
		posEnd = url.rfind(L'.');
		std::wstring id(url, posStart, posEnd - posStart);
		url = L"https://video.twimg.com/tweet_video/";
		url += id;
		url += L".mp4";
		WCHAR szTargetFilePath[MAX_PATH] = { 0 };
		if (lpszOutputFolder)
		{
			lstrcpyW(szTargetFilePath, lpszOutputFolder);
		}
		PathAppendW(szTargetFilePath, szUserName);
		lstrcatW(szTargetFilePath, L"_");
		lstrcatW(szTargetFilePath, szTweetID);
		lstrcatW(szTargetFilePath, L"_");
		lstrcatW(szTargetFilePath, id.c_str());
		lstrcatW(szTargetFilePath, L".mp4");
		if (PathFileExists(szTargetFilePath)) return 0;
		return DownloadToFile(url.c_str(), szTargetFilePath);
	}
	posStart += 40;
	posEnd = srcW.find(L'\"', posStart);
	std::wstring url(srcW, posStart, posEnd - posStart);
	lpszWeb = Download2WChar(url.c_str());
	if (!lpszWeb) return 0;
	srcW = lpszWeb;
	GlobalFree(lpszWeb);
	posStart = srcW.find(L"video_url&quot;:&quot;");
	if (posStart == std::wstring::npos) return 0;
	posStart += 22;
	posEnd = srcW.find(L'&', posStart);
	url = std::wstring(srcW, posStart, posEnd - posStart);
	url = Replace(url, L"\\", L"");
	LPBYTE lpszM3U8 = DownloadToMemory(url.c_str());
	if (!lpszM3U8) return 0;
	std::string srcA((char*)lpszM3U8);
	GlobalFree(lpszM3U8);
	std::vector<std::wstring> listResolution;
	std::vector<std::wstring> listURL;
	posStart = posEnd = 0;
	for (;;)
	{
		posStart = srcA.find("RESOLUTION=", posEnd);
		if (posStart == std::wstring::npos) break;
		posStart += 11;
		posEnd = srcA.find(',', posStart);
		if (posEnd == std::wstring::npos) break;
		{
			std::string strTemp = std::string(srcA, posStart, posEnd - posStart);
			const DWORD len = MultiByteToWideChar(CP_ACP, 0, strTemp.c_str(), -1, 0, 0);
			LPWSTR pwsz = (LPWSTR)GlobalAlloc(0, len * sizeof(WCHAR));
			MultiByteToWideChar(CP_ACP, 0, strTemp.c_str(), -1, pwsz, len);
			listResolution.push_back(std::wstring(pwsz));
			GlobalFree(pwsz);
		}
		posStart = srcA.find("\n", posEnd);
		if (posStart == std::wstring::npos) break;
		posStart += 1;
		posEnd = srcA.find('\n', posStart);
		if (posEnd == std::wstring::npos) break;
		{
			std::string strTemp = std::string("https://video.twimg.com") + std::string(srcA, posStart, posEnd - posStart);
			const DWORD len = MultiByteToWideChar(CP_ACP, 0, strTemp.c_str(), -1, 0, 0);
			LPWSTR pwsz = (LPWSTR)GlobalAlloc(0, len * sizeof(WCHAR));
			MultiByteToWideChar(CP_ACP, 0, strTemp.c_str(), -1, pwsz, len);
			listURL.push_back(std::wstring(pwsz));
			GlobalFree(pwsz);
		}
	}
	const int max = (int)listResolution.size();
	for (int i = max - 1; i < max; ++i)
	{
		WCHAR szTargetFilePath[MAX_PATH] = { 0 };
		if (lpszOutputFolder)
		{
			lstrcpyW(szTargetFilePath, lpszOutputFolder);
		}
		PathAppendW(szTargetFilePath, szUserName);
		lstrcatW(szTargetFilePath, L"_");
		lstrcatW(szTargetFilePath, szTweetID);
		lstrcatW(szTargetFilePath, L"_");
		lstrcatW(szTargetFilePath, listResolution[i].c_str());
		lstrcatW(szTargetFilePath, L".mp4");
		if (PathFileExists(szTargetFilePath)) continue;
		LPBYTE lpszM3U8 = DownloadToMemory(listURL[i].c_str());
		if (!lpszM3U8) continue;
		std::vector<std::wstring> tslist;
		{
			std::string srcA((char*)lpszM3U8);
			GlobalFree(lpszM3U8);
			posStart = posEnd = 0;
			for (;;)
			{
				posStart = srcA.find("#EXTINF", posEnd);
				if (posStart == std::wstring::npos) break;
				posStart += 7;
				posStart = srcA.find(',', posStart);
				if (posStart == std::wstring::npos) break;
				posStart += 1;
				posStart = srcA.find("\n", posStart);
				if (posStart == std::wstring::npos) break;
				posStart += 1;
				posEnd = srcA.find('\n', posStart);
				if (posEnd == std::wstring::npos) break;
				{
					std::string strTemp = std::string("https://video.twimg.com") + std::string(srcA, posStart, posEnd - posStart);
					const DWORD len = MultiByteToWideChar(CP_ACP, 0, strTemp.c_str(), -1, 0, 0);
					LPWSTR pwsz = (LPWSTR)GlobalAlloc(0, len * sizeof(WCHAR));
					MultiByteToWideChar(CP_ACP, 0, strTemp.c_str(), -1, pwsz, len);
					tslist.push_back(std::wstring(pwsz));
					GlobalFree(pwsz);
				}
			}
		}
		if (!tslist.size()) continue;
		WCHAR szTempDir[MAX_PATH];
		BOOL bIsSuccess = TRUE;
		if (CreateTempDirectory(szTempDir))
		{
			WCHAR szTsFilePath[MAX_PATH];
			for (auto v : tslist)
			{
				lstrcpyW(szTsFilePath, szTempDir);
				PathAppendW(szTsFilePath, GetFileName(v.c_str()));
				if (!DownloadToFile(v.c_str(), szTsFilePath))
				{
					bIsSuccess = FALSE;
					break;
				}
			}
			if (bIsSuccess)
			{
				WCHAR szListFile[MAX_PATH];
				lstrcpyW(szListFile, szTempDir);
				PathAppendW(szListFile, L"list.txt");
				HANDLE hFile = CreateFileW(szListFile, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					DWORD dwWriteSize;
					for (auto v : tslist)
					{
						WriteFile(hFile, "file ", 5, &dwWriteSize, 0);
						{
							const DWORD len = WideCharToMultiByte(CP_ACP, 0, GetFileName(v.c_str()), -1, 0, 0, 0, 0);
							char*psz = (char*)GlobalAlloc(GPTR, len * sizeof(char));
							WideCharToMultiByte(CP_ACP, 0, GetFileName(v.c_str()), -1, psz, len, 0, 0);
							WriteFile(hFile, psz, (DWORD)lstrlenA(psz), &dwWriteSize, 0);
							GlobalFree(psz);
						}
						WriteFile(hFile, "\r\n", 2, &dwWriteSize, 0);
					}
					CloseHandle(hFile);
					WCHAR szFFMpegPath[MAX_PATH];
					lstrcpyW(szFFMpegPath, szTempDir);
					PathAppendW(szFFMpegPath, L"ffmpeg.exe");
					MyCreateFileFromResource(MAKEINTRESOURCEW(IDR_EXE1), L"EXE", szFFMpegPath);
					PROCESS_INFORMATION pInfo;
					STARTUPINFOW sInfo;
					ZeroMemory(&sInfo, sizeof(STARTUPINFO));
					sInfo.cb = sizeof(STARTUPINFO);
					sInfo.dwFlags = STARTF_USEFILLATTRIBUTE | STARTF_USECOUNTCHARS | STARTF_USESHOWWINDOW;
					sInfo.wShowWindow = SW_HIDE;
					WCHAR szTempTsFilePath[MAX_PATH];
					lstrcpyW(szTempTsFilePath, szTempDir);
					PathAppendW(szTempTsFilePath, L"v1XNe1YgTJROqpHM.ts");
					WCHAR szTempMp4FilePath[MAX_PATH];
					lstrcpyW(szTempMp4FilePath, szTempDir);
					PathAppendW(szTempMp4FilePath, L"JFuzF3rwiKbxVfMv.mp4");
					WCHAR szCommandLine[1024];
					wsprintfW(szCommandLine, L"\"%s\" -f concat -i \"%s\" -c copy \"%s\"", szFFMpegPath, szListFile, szTempTsFilePath);
					CreateProcessW(0, szCommandLine, 0, 0, 0, 0, 0, szTempDir, &sInfo, &pInfo);
					CloseHandle(pInfo.hThread);
					WaitForSingleObject(pInfo.hProcess, INFINITE);
					CloseHandle(pInfo.hProcess);
					wsprintfW(szCommandLine, L"\"%s\" -i \"%s\" -vcodec copy -acodec copy -bsf:a aac_adtstoasc \"%s\"", szFFMpegPath, szTempTsFilePath, szTempMp4FilePath);
					CreateProcessW(0, szCommandLine, 0, 0, 0, 0, 0, szTempDir, &sInfo, &pInfo);
					CloseHandle(pInfo.hThread);
					WaitForSingleObject(pInfo.hProcess, INFINITE);
					CloseHandle(pInfo.hProcess);
					if (MoveFileExW(szTempMp4FilePath, szTargetFilePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
					{
						bReturnValue = TRUE;
					}
					else
					{
						DeleteFileW(szTempMp4FilePath);
					}
					DeleteFileW(szFFMpegPath);
					DeleteFileW(szListFile);
					DeleteFileW(szTempTsFilePath);
					for (auto v : tslist)
					{
						lstrcpyW(szTsFilePath, szTempDir);
						PathAppendW(szTsFilePath, GetFileName(v.c_str()));
						DeleteFileW(szTsFilePath);
					}
				}
			}
			RemoveDirectoryW(szTempDir);
		}
	}
	return bReturnValue;
}

BOOL GetVideoUrlList(HWND hList, LPCTSTR lpszUserID)
{
	TCHAR szUrl[1024] = { 0 };
	if (StrStr(lpszUserID, TEXT("//twitter.com/")) == NULL)
	{
		lstrcat(szUrl, TEXT("https://twitter.com/"));
	}
	lstrcat(szUrl, lpszUserID);
	LPWSTR lpszWeb = Download2WChar(szUrl);
	if (!lpszWeb) return 0;
	std::wstring srcW(lpszWeb);
	GlobalFree(lpszWeb);
	size_t posStart = 0, posEnd = 0;
	for (;;)
	{	
		posStart = srcW.find(L"<a href=\"", posStart);
		if (posStart == std::wstring::npos) break;
		posStart += 9;
		posEnd = srcW.find(L'\"', posStart);
		if (posEnd == std::wstring::npos) break;
		std::wstring strURL(srcW, posStart, posEnd - posStart);
		if ((strURL.find(L"/status/") != std::wstring::npos) || (strURL.find(L"//t.co/") != std::wstring::npos))
		{
			szUrl[0] = 0;
			if (strURL[0] == L'/')
			{
				lstrcat(szUrl, L"https://twitter.com");
			}
			lstrcat(szUrl, strURL.c_str());
			SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)szUrl);
		}
		posStart = posEnd;
	}
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit1, hEdit2, hButton1, hButton2, hButton3, hList;
	switch (msg)
	{
	case WM_CREATE:
		hEdit1 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT("SimonsCat"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 10, 10, 1024, 32, hWnd, (HMENU)1000, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hEdit2 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT("300000"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 10, 10, 1024, 32, hWnd, (HMENU)1001, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hButton1 = CreateWindow(TEXT("BUTTON"), TEXT("ダウンロード開始"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 10, 90, 256, 32, hWnd, (HMENU)1002, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hButton2 = CreateWindow(TEXT("BUTTON"), TEXT("定期的に監視して新しい動画がアップされていればダウンロードする"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 10, 130, 256, 32, hWnd, (HMENU)1003, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hButton3 = CreateWindow(TEXT("BUTTON"), TEXT("タイマーストップ"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED, 10, 170, 256, 32, hWnd, (HMENU)1004, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hList = CreateWindow(TEXT("LISTBOX"), 0, WS_CHILD | WS_DISABLED, 0, 0, 0, 0, hWnd, (HMENU)1005, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		break;
	case WM_SIZE:
		MoveWindow(hEdit1, 10, 10, lParam & 0xFFFF - 20, 32, TRUE);
		MoveWindow(hEdit2, 10, 50, lParam & 0xFFFF - 20, 32, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1002)
		{
			EnableWindow(hEdit1, FALSE);
			EnableWindow(hEdit2, FALSE);
			EnableWindow(hButton1, FALSE);
			EnableWindow(hButton2, FALSE);
			EnableWindow(hButton3, FALSE);
			DWORD dwSize = GetWindowTextLengthW(hEdit1);
			if (dwSize)
			{
				LPWSTR lpszInputUrl = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * (dwSize + 1));
				GetWindowTextW(hEdit1, lpszInputUrl, dwSize + 1);
				DownloadTwitterVideo(lpszInputUrl);
				GlobalFree(lpszInputUrl);
			}
			EnableWindow(hEdit1, TRUE);
			EnableWindow(hEdit2, TRUE);
			EnableWindow(hButton1, TRUE);
			EnableWindow(hButton2, TRUE);
			EnableWindow(hButton3, FALSE);
		}
		else if (LOWORD(wParam) == 1003)
		{
			EnableWindow(hEdit1, FALSE);
			EnableWindow(hEdit2, FALSE);
			EnableWindow(hButton1, FALSE);
			EnableWindow(hButton2, FALSE);
			EnableWindow(hButton3, TRUE);
			const int nTimerSpan = GetDlgItemInt(hWnd, 1001, 0, 0);
			SetTimer(hWnd, 0x1234, nTimerSpan, NULL);
		}
		else if (LOWORD(wParam) == 1004)
		{
			KillTimer(hWnd, 0x1234);
			EnableWindow(hEdit1, TRUE);
			EnableWindow(hEdit2, TRUE);
			EnableWindow(hButton1, TRUE);
			EnableWindow(hButton2, TRUE);
			EnableWindow(hButton3, FALSE);
		}
		break;
	case WM_TIMER:
		{
			KillTimer(hWnd, 0x1234);
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			TCHAR szUserID[256];
			GetWindowText(hEdit1, szUserID, _countof(szUserID));
			GetVideoUrlList(hList, szUserID);
			const int nSize = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
			for (int i = 0; i < nSize; ++i)
			{
				TCHAR szUrl[1024];
				SendMessage(hList, LB_GETTEXT, i, (LPARAM)szUrl);
				DownloadTwitterVideo(szUrl);
			}
			const int nTimerSpan = GetDlgItemInt(hWnd, 1001, 0, 0);
			SetTimer(hWnd, 0x1234, nTimerSpan, NULL);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		KillTimer(hWnd, 0x1234);
		PostQuitMessage(0);
		break;
	default:
		return DefDlgProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	int n;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &n);
	if (n == 1)
	{
		MSG msg;
		WNDCLASS wndclass = {
			CS_HREDRAW | CS_VREDRAW,
			WndProc,
			0,
			DLGWINDOWEXTRA,
			hInstance,
			0,
			LoadCursor(0,IDC_ARROW),
			0,
			0,
			szClassName
		};
		RegisterClass(&wndclass);
		HWND hWnd = CreateWindow(
			szClassName,
			TEXT("Twitterのツイートに含まれる動画をMP4形式でダウンロードする"),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			0,
			CW_USEDEFAULT,
			0,
			0,
			0,
			hInstance,
			0
		);
		ShowWindow(hWnd, SW_SHOWDEFAULT);
		UpdateWindow(hWnd);
		while (GetMessage(&msg, 0, 0, 0))
		{
			if (!IsDialogMessage(hWnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	else if (n == 2)
	{
		DownloadTwitterVideo(argv[1]);
	}
	else if (n >= 3)
	{
		DownloadTwitterVideo(argv[1], argv[2]);
	}
	LocalFree(argv);
	return 0;
}
