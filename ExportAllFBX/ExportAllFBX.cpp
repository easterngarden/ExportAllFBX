// ExportAllFBX.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <string>
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <codecvt>

void startup(LPCSTR lpApplicationName, LPSTR lpComand)
{
	// additional information
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// start the program up
	CreateProcessA
	(
		lpApplicationName,   // the path
		lpComand,                // Command line
		NULL,                   // Process handle not inheritable
		NULL,                   // Thread handle not inheritable
		FALSE,                  // Set handle inheritance to FALSE
		CREATE_NEW_CONSOLE,     // Opens file in a separate console
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi           // Pointer to PROCESS_INFORMATION structure
	);
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

int _tmain(int argc, TCHAR** argv)
{
	if (argc != 2)
	{
		_tprintf(TEXT("\nUsage: %s <directory name>\n"), argv[0]);
		return (-1);
	}

	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	DWORD dwError = 0;

	//FindFile .
	StringCchCopy(szDir, MAX_PATH, argv[1]);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(szDir, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("FindFirstFile failed (%d)\n", GetLastError());
		return -1;
	}

	std::wstring wExePath = std::wstring(argv[0]);
	std::wstring wFbxConverter = wExePath.substr(0, wExePath.rfind(_T("\\"))) +std::wstring(_T("\\")) + _T("FBXConverter.exe ");
	do {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			_tprintf(TEXT("  %s   <DIR>\n"), FindFileData.cFileName);
		else
		{
			_tprintf(TEXT("  %s \n"), FindFileData.cFileName);
			std::wstring wfilename = std::wstring(argv[1]) + std::wstring(_T("\\")) + FindFileData.cFileName;
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
			std::string filename = conv.to_bytes(wfilename);
			std::string fbxConverter = conv.to_bytes(wFbxConverter);

			std::string command = "start " + fbxConverter + std::string(" ") + filename + std::string(" "); 
			std::system(command.c_str());
		}
	} while (FindNextFile(hFind, &FindFileData) != 0);

	FindClose(hFind);
	dwError = GetLastError();
	return dwError;
}

