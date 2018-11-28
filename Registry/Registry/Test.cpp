#include <Windows.h>
#include <stdio.h>
#include "RegistryProcessor.h"

int main()
{
	HKEY test;
	DWORD dwData = 42;
	LPCSTR lpsData = "Hello";

	printf("%d\n", Registry::OpenKey(HKEY_CURRENT_USER, "Software\\MySoft\\TestKey", KEY_WRITE, &test));
	printf("%d\n", Registry::AddData(test, "", "TestInt", REG_DWORD, &dwData, sizeof(DWORD)));
	printf("%d\n", Registry::AddData(test, "subkey", "TestStr", REG_SZ, lpsData, lstrlen(lpsData) + 1));
	Registry::CloseKey(test);
	system("pause");
}
