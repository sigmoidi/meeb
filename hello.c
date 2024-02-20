#include <windows.h>

void entry() {
	DWORD tmp;
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "Hello!\n", 6, &tmp, 0);
}
