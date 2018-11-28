#include <Windows.h>
#include <stdio.h>
#include "RegistryProcessor.h"

int main()
{
	HKEY test;
	printf("%d\n", Registry::CreateKey(HKEY_CURRENT_USER, "Software\\MySoft\\TestKey"));
	printf("%d\n", Registry::OpenKey(HKEY_CURRENT_USER, "Software\\MySoft\\TestKey", KEY_READ, &test));
	system("pause");
}
