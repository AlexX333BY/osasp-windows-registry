#include "RegistryProcessor.h"
#include <Shlwapi.h>

namespace Registry
{
	typedef struct SearchRoutineData
	{
		HKEY hKey;
		LPCSTR lpsSearchQuery;
		DWORD dwResultSize;
		LPCSTR *lpsResult;
	};

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

	LPSTR *SingleLayerScan(HKEY hOpenedKey, LPCSTR lpsQuery, LPDWORD lpdwResultSize, LPCSTR lpsBasePath)
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
			if ((lLastStatus == ERROR_SUCCESS)/* && (StrStrI(lpsName, lpsQuery) != NULL)*/)
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

	LPSTR *RecursiveScan(HKEY hOpenedKey, LPCSTR lpsQuery, LPDWORD lpdwResultSize, LPCSTR lpsBasePath)
	{
		DWORD dwResultSize, dwSubresultSize, dwTotalSubresultSize = 0;

		LPSTR *lpsSubresult, *lpsTotalSubresult = (LPSTR *)calloc(0, sizeof(LPSTR));
		LPSTR *lpsBuffer;

		LPSTR *lpsResult = SingleLayerScan(hOpenedKey, lpsQuery, &dwResultSize, lpsBasePath);

		if (lpsResult == NULL)
		{
			return NULL;
		}

		for (DWORD dwResultElement = 0; dwResultElement < dwResultSize; ++dwResultElement)
		{
			lpsSubresult = RecursiveScan(hOpenedKey, lpsQuery, &dwSubresultSize, lpsResult[dwResultElement]);
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

	LPSTR *SearchForKeys(HKEY hOpenedKey, LPCSTR lpsQuery, LPDWORD lpdwResultSize)
	{
		if ((lpsQuery == NULL) || (lstrlen(lpsQuery) == 0) || (lpdwResultSize == NULL))
		{
			return NULL;
		}

		DWORD dwScanResultSize = 0;
		LPSTR *lpsSingleScan = SingleLayerScan(hOpenedKey, lpsQuery, &dwScanResultSize, "");

		LPSTR *lpsResult = SearchFor(lpsSingleScan, dwScanResultSize, lpsQuery, lpdwResultSize);
		free(lpsSingleScan);

		return lpsResult;
	}
}
