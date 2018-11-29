#pragma once

#include <Windows.h>

namespace Registry
{
	typedef struct _KEYFLAG
	{
		LPCSTR lpsFlagName;
		LPCSTR lpsFlagValue;
	} KEYFLAG;

	BOOL CreateKey(HKEY hOpenedKey, LPCSTR lpsRelativePath);
	BOOL OpenKey(HKEY hOpenedKey, LPCSTR lpsRelativePath, REGSAM samDesiredRights, PHKEY phResult);
	BOOL CloseKey(HKEY hKey);
	BOOL AddData(HKEY hOpenedKey, LPCSTR lpsRelativePath, LPCSTR lpsValueName, DWORD dwDataType, LPCVOID lpcData, DWORD dwDataSize);
	LPSTR *SearchForKeys(HKEY hOpenedKey, LPCSTR lpsQuery, LPDWORD lpdwResultSize);
	KEYFLAG *GetFlags(LPSTR lpsKey, LPDWORD lpdwFlagsCount);
	BOOL NotifyChange(HKEY hOpenedKey, LPSTR lpsRelativePath, BOOL bWatchSubtree);
}
