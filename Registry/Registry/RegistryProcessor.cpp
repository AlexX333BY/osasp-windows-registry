#include "RegistryProcessor.h"

namespace Registry
{
	BOOL CreateKey(HKEY hOpenedKey, LPCSTR lpsRelativePath)
	{
		if (lpsRelativePath == NULL)
		{
			return FALSE;
		}
		HKEY hKey;
		DWORD dwDisposition;
		LSTATUS lStatus = RegCreateKeyEx(hOpenedKey, lpsRelativePath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKey, &dwDisposition);
		CloseKey(hKey);
		return (lStatus == ERROR_SUCCESS) && (dwDisposition == REG_CREATED_NEW_KEY);
	}

	BOOL OpenKey(HKEY hOpenedKey, LPCSTR lpsRelativePath, REGSAM samDesiredRights, PHKEY phResult)
	{
		if ((lpsRelativePath == NULL) || (phResult == NULL))
		{
			return FALSE;
		}
		return RegOpenKeyEx(hOpenedKey, lpsRelativePath, 0, samDesiredRights, phResult) == ERROR_SUCCESS;
	}

	BOOL CloseKey(HKEY hKey)
	{
		return RegCloseKey(hKey) == ERROR_SUCCESS;
	}

	BOOL AddData(HKEY hOpenedKey, LPCSTR lpsRelativePath, LPCSTR lpsValueName, DWORD dwDataType, LPCVOID lpcData, DWORD dwDataSize)
	{
		if ((lpsValueName == NULL) || (lpcData == NULL))
		{
			return FALSE;
		}
		HKEY hWriteKey;
		if (!OpenKey(hOpenedKey, "", KEY_WRITE, &hWriteKey))
		{
			return FALSE;
		}
		LSTATUS lStatus = RegSetKeyValue(hWriteKey, lpsRelativePath, lpsValueName, dwDataType, lpcData, dwDataSize);
		CloseKey(hWriteKey);
		return lStatus == ERROR_SUCCESS;
	}

	LPSTR *SearchForKeys(HKEY hOpenedKey, LPCSTR lpsQuery, LPDWORD lpdwResultSize)
	{
		CONST WORD cwMaxNameLength = 256;

		if ((lpsQuery == NULL) || (lstrlen(lpsQuery) == 0) || (lpdwResultSize == NULL))
		{
			return NULL;
		}

		HKEY hSearchableKey;
		if (!OpenKey(hOpenedKey, "", KEY_ENUMERATE_SUB_KEYS, &hSearchableKey))
		{
			return NULL;
		}

		DWORD dwResultSize = 0;
		LPSTR *lpsResult = (LPSTR *)calloc(0, sizeof(LPSTR));
		LPSTR *lpsReallocatedResult;

		LPSTR lpsName = new CHAR[cwMaxNameLength];
		LPSTR lpsCopiedName;
		DWORD dwNameSize;

		LSTATUS lLastStatus = ERROR_SUCCESS;

		for (DWORD dwIndex = 0; lLastStatus != ERROR_NO_MORE_ITEMS; ++dwIndex)
		{
			dwNameSize = cwMaxNameLength;
			lLastStatus = RegEnumKeyEx(hSearchableKey, dwIndex, lpsName, &dwNameSize, 0, NULL, NULL, NULL);
			if (lLastStatus == ERROR_SUCCESS)
			{
				lpsCopiedName = (LPSTR)calloc(dwNameSize + 1, sizeof(CHAR));
				if (lpsCopiedName != NULL)
				{
					lpsReallocatedResult = (LPSTR *)realloc(lpsResult, dwResultSize + 1);
					if (lpsReallocatedResult != NULL)
					{
						lpsResult = lpsReallocatedResult;
						strcpy_s(lpsCopiedName, (dwNameSize + 1) * sizeof(CHAR), lpsName);
						lpsCopiedName[dwNameSize] = '\0';
						lpsResult[dwResultSize++] = lpsCopiedName;
					}
					else
					{
						free(lpsCopiedName);
					}
				}
			}
		}

		delete[] lpsName;
		*lpdwResultSize = dwResultSize;
		return lpsResult;
	}
}
