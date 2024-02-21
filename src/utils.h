#include "termcolor.h"

struct {
	HINSTANCE ntdll;
} __dll_table;

#define __dllcall(dll, proc, ...) \
	(GetProcAddress(__dll_table.dll, proc)(__VA_ARGS__))
#define __dllcall_casted(dll, proc, type, ...) \
	(((type)GetProcAddress(__dll_table.dll, proc))(__VA_ARGS__))

typedef int (*_ntdll_snprintf_t)(char*, size_t, const char*, ...);
typedef int (*_ntdll_vsnprintf_t)(char*, size_t, const char*, va_list);

#define __snprintf(buf, n, fmt, ...) \
	__dllcall_casted(ntdll, "_snprintf", _ntdll_snprintf_t, buf, n, fmt, __VA_ARGS__)

#define __vsnprintf(buf, n, fmt, args) \
	__dllcall_casted(ntdll, "_vsnprintf", _ntdll_vsnprintf_t, buf, n, fmt, args)

#define __error(fmt, ...) __printf(GREY "[" RED " OOPS " GREY "] " CRESET fmt "\r\n", __VA_ARGS__)
#if DEBUG == 1
#define __debug(fmt, ...) __printf(GREY "[ DBUG ] " fmt CRESET "\r\n", __VA_ARGS__)
#else
#define __debug(...) do {} while (0)
#endif
#define __info(fmt, ...) __printf(GREY "[" CRESET " INFO " GREY "] " CRESET fmt "\r\n", __VA_ARGS__)
#define __start(fmt, ...) __printf(GREY "[" YELLOW " WAIT " GREY "] " CRESET fmt "\r\n", __VA_ARGS__)
#define __ok(fmt, ...) __printf(GREY "[" GREEN " YEAH " GREY "] " CRESET fmt "\r\n", __VA_ARGS__)
#define __tool_output(fmt, ...) __printf(GREY "[" BLUE " TOOL " GREY "] " fmt CRESET "\r\n", __VA_ARGS__)

#define exit(code) ExitProcess(code)

void __printf(char* fmt, ...) {
	static char formatted[1024];
	static DWORD tmp;
	static va_list args;
	va_start(args, fmt);
	int count = __vsnprintf(formatted, 1024, fmt, args);
	va_end(args);
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), formatted, count, &tmp, 0);
}

void load_libraries() {
	__dll_table.ntdll = LoadLibrary("ntdll.dll");
}

size_t __strlen(char* str) {
	size_t l = 0;
	for (; *str++; l++);
	return l;
}

size_t __wstrlen(wchar_t* str) {
	size_t l = 0;
	for (; *str++; l++);
	return l;
}

bool __strcmp(char* a, char* b) {
	while (*a && *b) if (*a++ != *b++) return 0;
	return *a == 0 && *b == 0;
}

void __strcpy(char* dst, char* src) {
	for (; *src; *dst++ = *src++);
	*dst = 0;
}

void get_arguments(int* argc, char*** argv) {
	LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), argc);
	size_t total = 0;
	for (int i = 0; i < *argc; i++) {
		total += WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL) + 1;
	}
	*argv = HeapAlloc(GetProcessHeap(), 0, (*argc + 1) * sizeof(char*) + total);
	char* ptr = (char*)&((*argv)[*argc + 1]);
	for (int i = 0; i < *argc; i++) {
		(*argv)[i] = ptr;
		ptr += WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, ptr, total, NULL, NULL) + 1;
	}
	(*argv)[*argc] = 0;
}

void free_arguments(char** argv) {
	HeapFree(GetProcessHeap(), 0, argv);
}

/*char* where(char* name) {
	char* path = HeapAlloc(GetProcessHeap(), 0, 256);
	__strcpy(path, name);
	static const char* other_paths[2] = { ".\\tools", 0 };
	if (PathFindOnPathA(path, &other_paths)) return path;
	return 0;
}*/

void create_directory(char* name) {
	CreateDirectoryA(name, NULL);
}

void remove_directory(char* name) {
	char buf[256];
	size_t len = GetFullPathNameA(name, 254, buf, NULL);
	buf[len + 1] = 0; // SHFileOperation requires double-termination
	__debug("Deleting %s...", buf);
	SHFILEOPSTRUCT op = {
		NULL, FO_DELETE, buf, "",
		FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
		false, 0, ""
	};
	if (SHFileOperation(&op)) __debug("SHFileOperation failed");
}

bool file_exists(char* file) {
	DWORD dwAttrib = GetFileAttributes(file);
	return dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

#define STDOUT_BUFFER_SIZE 0x10000

bool run(char* fmt, ...) {
	bool success = true;

	char command[1024];
	va_list args;
	va_start(args, fmt);
	__vsnprintf(command, 1024, fmt, args);
	va_end(args);
	__debug("Full commandline: %s", command);

	HANDLE stdout_read = 0, stdout_write = 0;
	SECURITY_ATTRIBUTES sa;

	ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	CreatePipe(&stdout_read, &stdout_write, &sa, 0);
	SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.hStdError = stdout_write;
	si.hStdOutput = stdout_write;
	si.dwFlags |= STARTF_USESTDHANDLES;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	char* stdout_buffer = HeapAlloc(GetProcessHeap(), 0, STDOUT_BUFFER_SIZE);
	int chunk_read_size = 0, total_read = 0;
	char* stdout_line_start = stdout_buffer;

	if (!CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		__error("Could not run command! Make sure you have installed the required tools.");
		__error("Full commandline: " CYAN "%s" CRESET, command);
		__error("Please see the README for more information.");
		success = false;
		goto cleanup;
	}
	CloseHandle(stdout_write);
	while (total_read < STDOUT_BUFFER_SIZE) {
		if (!ReadFile(stdout_read, stdout_buffer + total_read, STDOUT_BUFFER_SIZE - total_read, &chunk_read_size, NULL)) {
			int err = GetLastError();
			if (err != ERROR_BROKEN_PIPE) {
				__error("ReadFile from pipe raised error code %d", err);
				success = false;
				goto cleanup;
			}
			break;
		}
		//__debug("Read %u bytes", chunk_read_size);
		char* prev_end = stdout_buffer + total_read;
		char* new_end = prev_end + chunk_read_size;
		for (char* c = prev_end; c < new_end; c++) {
			if (*c == '\n') {
				*c = 0;
				__tool_output("%s", stdout_line_start);
				stdout_line_start = c + 1;
				//*c = '\n';
			}
		}
		total_read += chunk_read_size;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	int exit_code;
	GetExitCodeProcess(pi.hProcess, &exit_code);
	__debug("Process exited with code %d", exit_code);

	if (exit_code != 0) {
		__error("Error in child process!");
		__error("Full commandline: " CYAN "%s" CRESET, command);
		success = false;
	}

cleanup:
	if (pi.hProcess)   CloseHandle(pi.hProcess);
	if (pi.hThread)    CloseHandle(pi.hThread);
	if (stdout_read)   CloseHandle(stdout_read);
	if (stdout_buffer) HeapFree(GetProcessHeap(), 0, stdout_buffer);

	return success;
}

void print_options() {
	__printf(GREY "Usage:" CRESET " meeb " CYAN "source " GREEN "[options]" CRESET "\n"
	         GREY "Options:\n" CRESET
	         GREEN "   -o" CRESET "\tOutput file\n"
	         GREEN "   -o!" CRESET "\tOutput file, force overwrite\n"
	         GREY "\tDefaults to 'out.exe', no force\n" CRESET
	         GREEN "   -c" CRESET "\tCompiler, either 'msvc' or 'gcc'\n"
	         GREY "\tDefaults to 'msvc'\n" CRESET
	         GREEN "   -l" CRESET "\tLinker, either 'msvc' or 'crinkler'\n"
	         GREY "\tDefaults to 'msvc'\n" CRESET
	         GREEN "   -x" CRESET "\tFurther compression, either 'upx' or 'kkrunchy'\n"
	         GREY "\tDefaults to none\n" CRESET
	         GREEN "   -f" CRESET "\tOptimization goal, either 'fast' or 'small'\n"
	         GREY "\tDefaults to 'fast'\n" CRESET
	         GREEN "   -b" CRESET "\tExecutable bitness, either 32 or 64\n"
	         GREY "\tDefaults to 64\n" CRESET
	         GREEN "   -d" CRESET "\tDon't strip or remove debugging information\n"
	         GREY "\tDefaults to stripped (no flag)\n" CRESET);
}

void print_help() {
	__printf(CYAN
		     " ____    ____  ________  ________  _______\n"
	         "|_   \\  /   _||_   __  ||_   __  ||_   _  \\\n"
	         "  |   \\/   |    | |_ \\_|  | |_ \\_|  | |_) /\n"
	         "  | |\\  /| |    |  _| _   |  _| _   |  __ \\\n"
	         " _| |_\\/_| |_  _| |__/ | _| |__/ | _| |__) |\n"
	         "|_____||_____||________||________||_______/\n"
	         CRESET);

	__printf("\n==== The " CYAN "M" CRESET "inimal " CYAN "E" CRESET "x" CYAN "e" CRESET "cutable " CYAN "B" CRESET "uild Tool ====\n"
		     "            Written by Henri A.\n\n");

	print_options();

	__printf(RED "\nNote:" CRESET " You must install any desired tools beforehand.\n"
	                         "      Please see the README for more information.\n");
}
