#include "RegistryProcessor.h"
#include <Shlwapi.h>
#include <stdio.h>

namespace Registry
{
	typedef struct _SEARCHROUTINEDATA
	{
		HKEY hKey;
		LPCSTR lpsSearchQuery;
		LPCSTR lpsBasePath;
		DWORD dwResultSize;
		LPSTR *lpsResult;
	} SEARCHROUTINEDATA;

	typedef struct _THREADSEARCHDATAPAIR
	{
		HANDLE hThread;
		SEARCHROUTINEDATA srdData;
	} THREADSEARCHDATAPAIR;

	CONST WORD cwMaxNameLength = 256;

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

	LPSTR *ConcatLpstrArrays(LPSTR *lpsMainArray, DWORD dwMainArrayElementsCount, LPSTR *lpsAddition, DWORD dwAdditionElementsCount)
	{
		if ((lpsAddition == NULL) || (dwAdditionElementsCount == 0))
		{
			return lpsMainArray;
		}
		if (lpsMainArray == NULL)
		{
			return NULL;
		}
		LPSTR *lpsResultArray = (LPSTR *)realloc(lpsMainArray, (dwMainArrayElementsCount + dwAdditionElementsCount) * sizeof(LPSTR));
		if (lpsResultArray == NULL)
		{
			return NULL;
		}
		for (DWORD dwAdditionElement = 0; dwAdditionElement < dwAdditionElementsCount; ++dwAdditionElement)
		{
			lpsResultArray[dwMainArrayElementsCount + dwAdditionElement] = lpsAddition[dwAdditionElement];
		}
		return lpsResultArray;
	}

	LPSTR CreateSplittedName(LPCSTR lpsBasePath, LPCSTR lpsName)
	{
		DWORD dwOldLength = lstrlen(lpsBasePath), dwAdditionLength = lstrlen(lpsName), dwNewLength;
		LPSTR lpsResult;
		if (dwOldLength == 0)
		{
			dwNewLength = dwAdditionLength;
			lpsResult = (LPSTR)calloc(dwNewLength + 1, sizeof(CHAR));
			if (lpsResult != NULL)
			{
				strcpy_s(lpsResult, dwNewLength + 1, lpsName);
				lpsResult[dwNewLength] = '\0';
			}
		}
		else
		{
			dwNewLength = dwOldLength + 1 + dwAdditionLength;
			lpsResult = (LPSTR)calloc(dwNewLength + 1, sizeof(CHAR));
			if (lpsResult != NULL)
			{
				strcpy_s(lpsResult, dwOldLength + 1, lpsBasePath);
				lpsResult[dwOldLength] = '\\';
				strcpy_s(lpsResult + dwOldLength + 1, dwAdditionLength + 1, lpsName);
			}
		}
		return lpsResult;
	}

	LPSTR *SingleLayerScan(HKEY hOpenedKey, LPDWORD lpdwResultSize, LPCSTR lpsBasePath)
	{
		HKEY hSearchableKey;
		if (!OpenKey(hOpenedKey, lpsBasePath, KEY_ENUMERATE_SUB_KEYS, &hSearchableKey))
		{
			return NULL;
		}

		DWORD dwResultSize = 0;
		LPSTR *lpsResult = (LPSTR *)calloc(0, sizeof(LPSTR));
		LPSTR *lpsReallocatedResult;

		LPSTR lpsName = new CHAR[cwMaxNameLength];
		LPSTR lpsFullName;
		DWORD dwNameSize;

		LSTATUS lLastStatus = ERROR_SUCCESS;

		for (DWORD dwIndex = 0; lLastStatus != ERROR_NO_MORE_ITEMS; ++dwIndex)
		{
			dwNameSize = cwMaxNameLength;
			lLastStatus = RegEnumKeyEx(hSearchableKey, dwIndex, lpsName, &dwNameSize, 0, NULL, NULL, NULL);
			if (lLastStatus == ERROR_SUCCESS)
			{
				lpsFullName = CreateSplittedName(lpsBasePath, lpsName);
				if (lpsFullName != NULL)
				{
					lpsReallocatedResult = ConcatLpstrArrays(lpsResult, dwResultSize, &lpsFullName, 1);
					if (lpsReallocatedResult != NULL)
					{
						lpsResult = lpsReallocatedResult;
						++dwResultSize;
					}
					else
					{
						free(lpsFullName);
					}
				}
			}
		}

		delete[] lpsName;
		CloseKey(hSearchableKey);
		*lpdwResultSize = dwResultSize;
		return lpsResult;
	}

	LPSTR *RecursiveScan(HKEY hOpenedKey, LPDWORD lpdwResultSize, LPCSTR lpsBasePath)
	{
		DWORD dwResultSize, dwSubresultSize, dwTotalSubresultSize = 0;

		LPSTR *lpsSubresult, *lpsTotalSubresult = (LPSTR *)calloc(0, sizeof(LPSTR));
		LPSTR *lpsBuffer;

		LPSTR *lpsResult = SingleLayerScan(hOpenedKey, &dwResultSize, lpsBasePath);

		if (lpsResult == NULL)
		{
			return NULL;
		}

		for (DWORD dwResultElement = 0; dwResultElement < dwResultSize; ++dwResultElement)
		{
			lpsSubresult = RecursiveScan(hOpenedKey, &dwSubresultSize, lpsResult[dwResultElement]);
			if (lpsSubresult != NULL)
			{
				lpsBuffer = ConcatLpstrArrays(lpsTotalSubresult, dwTotalSubresultSize, lpsSubresult, dwSubresultSize);
				if (lpsBuffer != NULL)
				{
					lpsTotalSubresult = lpsBuffer;
					dwTotalSubresultSize += dwSubresultSize;
				}
				free(lpsSubresult);
			}
		}

		lpsBuffer = ConcatLpstrArrays(lpsResult, dwResultSize, lpsTotalSubresult, dwTotalSubresultSize);
		free(lpsTotalSubresult);
		if (lpsBuffer == NULL)
		{
			*lpdwResultSize = dwResultSize;
			return lpsResult;
		}
		else
		{
			*lpdwResultSize = dwResultSize + dwTotalSubresultSize;
			return lpsBuffer;
		}
	}

	LPSTR *SearchFor(LPSTR *lpsTargets, DWORD dwTargetsSize, LPCSTR lpsSearchQuery, LPDWORD lpdwResultSize)
	{
		DWORD dwResultSize = 0;
		LPSTR *lpsResult = (LPSTR *)calloc(dwResultSize, sizeof(LPSTR)), *lpsBuffer;
		if (lpsResult != NULL)
		{
			for (DWORD dwCurTarget = 0; dwCurTarget < dwTargetsSize; ++dwCurTarget)
			{
				if (StrStrI(lpsTargets[dwCurTarget], lpsSearchQuery) != NULL)
				{
					lpsBuffer = ConcatLpstrArrays(lpsResult, dwResultSize, &(lpsTargets[dwCurTarget]), 1);
					if (lpsBuffer != NULL)
					{
						lpsResult = lpsBuffer;
						++dwResultSize;
					}
				}
			}
		}
		*lpdwResultSize = dwResultSize;
		return lpsResult;
	}

	DWORD WINAPI ScanAndSearchThreadRoutine(LPVOID lpParam)
	{
		SEARCHROUTINEDATA *srdData = (SEARCHROUTINEDATA *)lpParam;
		DWORD dwScanResultCount;
		LPSTR *lpsScanResult = RecursiveScan(srdData->hKey, &dwScanResultCount, srdData->lpsBasePath);

		if (lpsScanResult == NULL)
		{
			srdData->lpsResult = NULL;
			return 1;
		}
		else
		{
			srdData->lpsResult = SearchFor(lpsScanResult, dwScanResultCount, srdData->lpsSearchQuery, &srdData->dwResultSize);
			free(lpsScanResult);
			return 0;
		}
	}

	LPSTR *SearchForKeys(HKEY hOpenedKey, LPCSTR lpsQuery, LPDWORD lpdwResultSize)
	{
		if ((lpsQuery == NULL) || (lstrlen(lpsQuery) == 0) || (lpdwResultSize == NULL))
		{
			return NULL;
		}

		DWORD dwScanResultSize = 0;
		LPSTR *lpsSingleScan = SingleLayerScan(hOpenedKey, &dwScanResultSize, "");

		if (lpsSingleScan == NULL)
		{
			return NULL;
		}

		THREADSEARCHDATAPAIR *tsdpSearchStruct = (THREADSEARCHDATAPAIR *)calloc(dwScanResultSize, sizeof(THREADSEARCHDATAPAIR));
		for (DWORD dwResultItem = 0; dwResultItem < dwScanResultSize; ++dwResultItem)
		{
			tsdpSearchStruct[dwResultItem].srdData.hKey = hOpenedKey;
			tsdpSearchStruct[dwResultItem].srdData.lpsBasePath = lpsSingleScan[dwResultItem];
			tsdpSearchStruct[dwResultItem].srdData.lpsSearchQuery = lpsQuery;
			tsdpSearchStruct[dwResultItem].hThread = CreateThread(NULL, 0, ScanAndSearchThreadRoutine, &tsdpSearchStruct[dwResultItem].srdData, 0, NULL);
		}

		DWORD dwResultSize;
		LPSTR *lpsResult = SearchFor(lpsSingleScan, dwScanResultSize, lpsQuery, &dwResultSize), *lpsBuffer;

		for (DWORD dwThreadItem = 0; dwThreadItem < dwScanResultSize; ++dwThreadItem)
		{
			if (tsdpSearchStruct[dwThreadItem].hThread != NULL)
			{
				WaitForSingleObject(tsdpSearchStruct[dwThreadItem].hThread, INFINITE);
				if (tsdpSearchStruct[dwThreadItem].srdData.lpsResult != NULL)
				{
					lpsBuffer = ConcatLpstrArrays(lpsResult, dwResultSize, tsdpSearchStruct[dwThreadItem].srdData.lpsResult, tsdpSearchStruct[dwThreadItem].srdData.dwResultSize);
					if (lpsBuffer != NULL)
					{
						lpsResult = lpsBuffer;
						dwResultSize += tsdpSearchStruct[dwThreadItem].srdData.dwResultSize;
					}
					free(tsdpSearchStruct[dwThreadItem].srdData.lpsResult);
				}
				CloseHandle(tsdpSearchStruct[dwThreadItem].hThread);
			}
		}

		free(lpsSingleScan);
		*lpdwResultSize = dwResultSize;

		return lpsResult;
	}

	KEYFLAG *GetInitializedFlags(LPDWORD lpdwCount)
	{
		CONST DWORD dwFlagsCount = 3;
		KEYFLAG *kfResult = (KEYFLAG *)calloc(dwFlagsCount, sizeof(KEYFLAG));

		if (kfResult == NULL)
		{
			return NULL;
		}

		kfResult[0].lpsFlagName = "REG_KEY_DONT_VIRTUALIZE";
		kfResult[1].lpsFlagName = "REG_KEY_DONT_SILENT_FAIL";
		kfResult[2].lpsFlagName = "REG_KEY_RECURSE_FLAG";
		*lpdwCount = dwFlagsCount;
		return kfResult;
	}

	LPSTR GetCommandOutput(LPSTR lpsCommand)
	{
		HANDLE hReadPipe, hWritePipe;
		SECURITY_ATTRIBUTES saAttributes;
		saAttributes.bInheritHandle = TRUE;
		saAttributes.lpSecurityDescriptor = NULL;
		saAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
		LPSTR lpsResult = NULL;

		if (CreatePipe(&hReadPipe, &hWritePipe, &saAttributes, 0))
		{
			STARTUPINFO siConsole;
			ZeroMemory(&siConsole, sizeof(STARTUPINFO));
			siConsole.hStdOutput = hWritePipe;
			siConsole.hStdError = hWritePipe;
			siConsole.hStdInput = hReadPipe;
			siConsole.dwFlags = STARTF_USESTDHANDLES;
			PROCESS_INFORMATION piInfo;

			if (CreateProcess(NULL, lpsCommand, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siConsole, &piInfo))
			{
				CONST DWORD dwBufLength = 4096;
				LPSTR lpsBuffer = (LPSTR)calloc(dwBufLength, sizeof(CHAR));
				DWORD dwReadCount;

				WaitForSingleObject(piInfo.hProcess, INFINITE);

				if (ReadFile(hReadPipe, lpsBuffer, (dwBufLength - 1) * sizeof(CHAR), &dwReadCount, NULL))
				{
					lpsBuffer[dwReadCount / sizeof(CHAR)] = '\0';
					lpsResult = lpsBuffer;
				}
				else
				{
					free(lpsBuffer);
				}

				CloseHandle(piInfo.hThread);
				CloseHandle(piInfo.hProcess);
			}
			CloseHandle(hReadPipe);
			CloseHandle(hWritePipe);
		}

		return lpsResult;
	}

	BOOL ParseFlagsOutput(LPSTR lpsCommandOutput, KEYFLAG *kfFlags, DWORD dwKeyCount)
	{
		if ((lpsCommandOutput == NULL) || (kfFlags == NULL))
		{
			return FALSE;
		}

		LPSTR lpsKeyPos, lpsKeyValuePos, lpsBuffer;
		LPCSTR lpsDelimiters = ": \r\t\n";

		DWORD dwCommandOutputLength = lstrlen(lpsCommandOutput);
		LPSTR lpsCommandOutputCopy = (LPSTR)calloc(dwCommandOutputLength + 1, sizeof(CHAR));

		DWORD dwValueLength;

		for (DWORD dwCurKey = 0; dwCurKey < dwKeyCount; ++dwCurKey)
		{
			strcpy_s(lpsCommandOutputCopy, (dwCommandOutputLength + 1) * sizeof(CHAR), lpsCommandOutput);
			lpsKeyPos = StrStrI(lpsCommandOutputCopy, kfFlags[dwCurKey].lpsFlagName);
			if (lpsKeyPos == NULL)
			{
				kfFlags[dwCurKey].lpsFlagValue = "";
			}
			else
			{
				lpsKeyValuePos = strtok_s(lpsKeyPos + lstrlen(kfFlags[dwCurKey].lpsFlagName), lpsDelimiters, &lpsBuffer);
				if (lpsKeyValuePos == NULL)
				{
					kfFlags[dwCurKey].lpsFlagValue = "";
				}
				else
				{
					dwValueLength = lstrlen(lpsKeyValuePos);
					lpsBuffer = (LPSTR)calloc(dwValueLength + 1, sizeof(CHAR));
					if (lpsBuffer == NULL)
					{
						kfFlags[dwCurKey].lpsFlagValue = "";
					}
					else
					{
						strcpy_s(lpsBuffer, dwValueLength + 1, lpsKeyValuePos);
						kfFlags[dwCurKey].lpsFlagValue = lpsBuffer;
					}
				}
			}
		}

		free(lpsCommandOutputCopy);
		return TRUE;
	}

	KEYFLAG *GetFlags(LPSTR lpsKey, LPDWORD lpdwFlagsCount)
	{
		if ((lpsKey == NULL) || (lpdwFlagsCount == NULL))
		{
			return NULL;
		}
		DWORD dwFlagsCount;
		KEYFLAG *kfResult = GetInitializedFlags(&dwFlagsCount);
		if (kfResult == NULL)
		{
			return NULL;
		}

		DWORD dwCmdLength = lstrlen("REG FLAGS  QUERY") + lstrlen(lpsKey);
		LPSTR lpsCmd = (LPSTR)calloc(dwCmdLength + 1, sizeof(CHAR));
		if (lpsCmd == NULL)
		{
			return NULL;
		}
		sprintf_s(lpsCmd, dwCmdLength + 1, "REG FLAGS %s QUERY", lpsKey);
		LPSTR lpsCmdOutput = GetCommandOutput(lpsCmd);
		free(lpsCmd);
		if (lpsCmdOutput == NULL)
		{
			return NULL;
		}

		if (ParseFlagsOutput(lpsCmdOutput, kfResult, dwFlagsCount))
		{
			*lpdwFlagsCount = dwFlagsCount;
			return kfResult;
		}
		else
		{
			return NULL;
		}
		
		return kfResult;
	}
}
