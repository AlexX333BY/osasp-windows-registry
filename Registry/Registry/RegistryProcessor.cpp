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
		return RegSetKeyValue(hOpenedKey, lpsRelativePath, lpsValueName, dwDataType, lpcData, dwDataSize) == ERROR_SUCCESS;
	}
}
