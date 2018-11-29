#include <Windows.h>
#include <stdio.h>
#include "RegistryProcessor.h"

int main()
{
	HKEY test;
	DWORD dwResSize = 0;

	Registry::OpenKey(HKEY_CURRENT_USER, "Software", KEY_WRITE, &test);
	Registry::KEYFLAG *res = Registry::GetFlags((LPSTR)"HKLM\\Software", &dwResSize);
	/*LPSTR *res = Registry::SearchForKeys(test, "Go", &dwResSize);*/
	for (DWORD i = 0; i < dwResSize; i++)
	{
		printf("%s: %s\n", res[i].lpsFlagName, res[i].lpsFlagValue);
	}
	Registry::CloseKey(test);
	system("pause");
}
