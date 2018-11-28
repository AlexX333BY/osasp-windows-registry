#pragma once

#include <Windows.h>

namespace Registry
{
	BOOL CreateKey(HKEY hOpenedKey, LPCSTR lpsRelativePath);
	BOOL OpenKey(HKEY hOpenedKey, LPCSTR lpsRelativePath, REGSAM samDesiredRights, PHKEY phResult);
	BOOL CloseKey(HKEY hKey);
	BOOL AddData(HKEY hOpenedKey, LPCSTR lpsRelativePath, LPCSTR lpsValueName, DWORD dwDataType, LPCVOID lpcData, DWORD dwDataSize);
}
