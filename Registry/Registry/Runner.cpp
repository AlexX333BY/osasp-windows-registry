#include <Windows.h>
#include <stdio.h>
#include "RegistryProcessor.h"

LPCSTR SuccessMessage = "Success";

LPCSTR FailMessage = "Fail";


HKEY GetHkeyByString(LPSTR lpsKey)
{
	if (_stricmp(lpsKey, "HKEY_CLASSES_ROOT") == 0)
	{
		return HKEY_CLASSES_ROOT;
	}
	if (_stricmp(lpsKey, "HKEY_CURRENT_USER") == 0)
	{
		return HKEY_CURRENT_USER;
	}
	if (_stricmp(lpsKey, "HKEY_LOCAL_MACHINE") == 0)
	{
		return HKEY_LOCAL_MACHINE;
	}
	if (_stricmp(lpsKey, "HKEY_USERS") == 0)
	{
		return HKEY_USERS;
	}
	if (_stricmp(lpsKey, "HKEY_CURRENT_CONFIG") == 0)
	{
		return HKEY_CURRENT_CONFIG;
	}
	return NULL;
}

LPSTR GetHkeyFromPath(LPSTR lpsFullPath)
{
	LPSTR lpsResult;
	DWORD dwResultLength;
	LPSTR lpsDelim = strchr(lpsFullPath, '\\');
	if (lpsDelim == NULL)
	{
		dwResultLength = lstrlen(lpsFullPath);
		lpsResult = (LPSTR)calloc(dwResultLength + 1, sizeof(CHAR));
		strcpy_s(lpsResult, dwResultLength + 1, lpsFullPath);
	}
	else
	{
		dwResultLength = lpsDelim - lpsFullPath;
		lpsResult = (LPSTR)calloc(dwResultLength + 1, sizeof(CHAR));
		strncpy_s(lpsResult, (dwResultLength + 1) * sizeof(CHAR), lpsFullPath, dwResultLength);
	}

	return lpsResult;
}

LPCSTR AddDataCommand(LPSTR *lpsArguments, DWORD dwArgumentsCount)
{
	if (dwArgumentsCount < 3)
	{
		return FailMessage;
	}

	if (Registry::AddData(HKEY_CURRENT_USER, lpsArguments[0], lpsArguments[1], REG_SZ, lpsArguments[2], lstrlen(lpsArguments[2])))
	{
		return SuccessMessage;
	}
	else
	{
		return FailMessage;
	}
}

LPCSTR CreateKeyCommand(LPSTR *lpsArguments, DWORD dwArgumentsCount)
{
	if (dwArgumentsCount < 1)
	{
		return FailMessage;
	}

	LPSTR lpsHkey = GetHkeyFromPath(lpsArguments[0]);
	HKEY hKey = GetHkeyByString(lpsHkey);
	if (hKey == NULL)
	{
		return FailMessage;
	}

	LPSTR lpsRelativePath;
	DWORD dwKeyLength = lstrlen(lpsHkey);
	if (dwKeyLength == lstrlen(lpsArguments[0]))
	{
		lpsRelativePath = (LPSTR)"";
	}
	else
	{
		lpsRelativePath = lpsArguments[0] + dwKeyLength + 1;
	}

	if (Registry::CreateKey(hKey, lpsRelativePath))
	{
		return SuccessMessage;
	}
	else
	{
		return FailMessage;
	}
}

LPCSTR GetFlagsCommand(LPSTR *lpsArguments, DWORD dwArgumentsCount)
{
	if (dwArgumentsCount < 1)
	{
		return FailMessage;
	}

	DWORD dwResultCount;
	Registry::KEYFLAG * kfFlags = Registry::GetFlags(lpsArguments[0], &dwResultCount);
	if (kfFlags == NULL)
	{
		return FailMessage;
	}
	else
	{
		DWORD dwResultLength = 0;
		for (DWORD dwCurFlag = 0; dwCurFlag < dwResultCount; ++dwCurFlag)
		{
			dwResultLength += lstrlen(kfFlags[dwCurFlag].lpsFlagName) + lstrlen(kfFlags[dwCurFlag].lpsFlagValue) + 3;
		}

		LPSTR lpsResult = (LPSTR)calloc(dwResultLength + 1, sizeof(CHAR));

		for (DWORD dwCurFlag = 0; dwCurFlag < dwResultCount; ++dwCurFlag)
		{
			strcat_s(lpsResult, dwResultLength + 1, kfFlags[dwCurFlag].lpsFlagName);
			strcat_s(lpsResult, dwResultLength + 1, ": ");
			strcat_s(lpsResult, dwResultLength + 1, kfFlags[dwCurFlag].lpsFlagValue);
			strcat_s(lpsResult, dwResultLength + 1, "\n");
		}
		free(kfFlags);
		return lpsResult;
	}
}

LPCSTR SearchCommand(LPSTR *lpsArguments, DWORD dwArgumentsCount)
{
	if (dwArgumentsCount < 2)
	{
		return FailMessage;
	}

	LPSTR lpsHkey = GetHkeyFromPath(lpsArguments[0]);
	HKEY hKey = GetHkeyByString(lpsHkey);
	if (hKey == NULL)
	{
		return FailMessage;
	}

	LPSTR lpsRelativePath;
	DWORD dwKeyLength = lstrlen(lpsHkey);
	if (dwKeyLength == lstrlen(lpsArguments[0]))
	{
		lpsRelativePath = (LPSTR)"";
	}
	else
	{
		lpsRelativePath = lpsArguments[0] + dwKeyLength + 1;
	}

	HKEY hOpenedKey;
	if (!Registry::OpenKey(hKey, lpsRelativePath, KEY_ENUMERATE_SUB_KEYS, &hOpenedKey))
	{
		return FailMessage;
	}

	DWORD dwResultSize;
	LPSTR *lpsSearchResult = Registry::SearchForKeys(hOpenedKey, lpsArguments[1], &dwResultSize);
	if (lpsSearchResult == NULL)
	{
		return FailMessage;
	}

	DWORD dwResultLength = 0;

	for (DWORD dwCurResult = 0; dwCurResult < dwResultSize; ++dwCurResult)
	{
		dwResultLength += lstrlen(lpsSearchResult[dwCurResult]) + 1;
	}

	LPSTR lpsResult = (LPSTR)calloc(dwResultLength + 1, sizeof(CHAR));
	if (lpsResult == NULL)
	{
		return FailMessage;
	}

	for (DWORD dwCurResult = 0; dwCurResult < dwResultSize; ++dwCurResult)
	{
		strcat_s(lpsResult, dwResultLength + 1, lpsSearchResult[dwCurResult]);
		strcat_s(lpsResult, dwResultLength + 1, "\n");
	}
	free(lpsSearchResult);
	return lpsResult;
}

LPCSTR NotifyCommand(LPSTR *lpsArguments, DWORD dwArgumentsCount)
{
	if (dwArgumentsCount < 1)
	{
		return FailMessage;
	}

	LPSTR lpsHkey = GetHkeyFromPath(lpsArguments[0]);
	HKEY hKey = GetHkeyByString(lpsHkey);
	if (hKey == NULL)
	{
		return FailMessage;
	}

	LPSTR lpsRelativePath;
	DWORD dwKeyLength = lstrlen(lpsHkey);
	if (dwKeyLength == lstrlen(lpsArguments[0]))
	{
		lpsRelativePath = (LPSTR)"";
	}
	else
	{
		lpsRelativePath = lpsArguments[0] + dwKeyLength + 1;
	}

	if (Registry::NotifyChange(hKey, lpsRelativePath, TRUE))
	{
		return SuccessMessage;
	}
	else
	{
		return FailMessage;
	}
}

LPCSTR CommandController(LPSTR *lpsArguments, DWORD dwArgumentsCount)
{
	if (dwArgumentsCount < 2)
	{
		return FailMessage;
	}

	if (_stricmp(lpsArguments[1], "ADD") == 0)
	{
		return AddDataCommand(lpsArguments + 2, dwArgumentsCount - 2);
	}
	if (_stricmp(lpsArguments[1], "CREATE_KEY") == 0)
	{
		return CreateKeyCommand(lpsArguments + 2, dwArgumentsCount - 2);
	}
	if (_stricmp(lpsArguments[1], "FLAGS") == 0)
	{
		return GetFlagsCommand(lpsArguments + 2, dwArgumentsCount - 2);
	}
	if (_stricmp(lpsArguments[1], "SEARCH") == 0)
	{
		return SearchCommand(lpsArguments + 2, dwArgumentsCount - 2);
	}
	if (_stricmp(lpsArguments[1], "NOTIFY") == 0)
	{
		return NotifyCommand(lpsArguments + 2, dwArgumentsCount - 2);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	LPCSTR lpsCmdResult = CommandController(argv, argc);
	if (lpsCmdResult == NULL)
	{
		printf("No such command\n");
	}
	else
	{
		printf("%s\n", lpsCmdResult);
	}
	return 0;
}
