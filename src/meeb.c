#include <windows.h>
#include <inttypes.h>

#define true 1
#define false 0
typedef uint_fast8_t bool;

#define DEBUG false

#include "utils.h"

enum CompilerOpt { COMPILER_MSVC, COMPILER_GCC };
enum LinkerOpt { LINKER_MSVC, LINKER_CRINKLER };
enum CompressionOpt { COMPRESSION_NONE, COMPRESSION_UPX, COMPRESSION_KKRUNCHY };

#define MSVC_COMPILE_FLAGS "/nologo /GS- /c"
#define MSVC_LINK_FLAGS "/nologo /SUBSYSTEM:CONSOLE /NODEFAULTLIB /ENTRY:entry kernel32.lib ntdll.lib shell32.lib shlwapi.lib user32.lib"

#define MSVC_COMPILE_FAST "/O2 /Ot"
#define MSVC_COMPILE_SMALL "/O1 /Os"
#define MSVC_LINK_FAST ""
#define MSVC_LINK_SMALL "/ALIGN:16 /FILEALIGN:16"

#define GCC_COMPILE_FLAGS "-std=c17 -nostdlib -nostartfiles -c"

#define GCC_COMPILE_FAST "-O3"
#define GCC_COMPILE_SMALL "-Os"

#define CRINKLER_LINK_FLAGS "/CRINKLER /SUBSYSTEM:CONSOLE /NODEFAULTLIB /ENTRY:entry kernel32.lib ntdll.lib shell32.lib shlwapi.lib user32.lib"

int main() {
	int err = 0;

	int argc;
	char** argv;

	load_libraries();
	get_arguments(&argc, &argv);

	if (argc < 2) goto help;

	struct {
		char* source;
		char* output;
		bool force;
		enum CompilerOpt compiler;
		enum LinkerOpt linker;
		enum CompressionOpt compression;
		bool strip;
		bool x64;
		bool tiny;
	} options = {0, "program.exe", false, COMPILER_MSVC, LINKER_MSVC, COMPRESSION_NONE, true, true, false};

	for (int i = 1; i < argc; i++) {
		if (!options.source && (__strlen(argv[i]) > 2 || argv[i][0] != '-')) {
			options.source = argv[i];
		} else if (__strcmp(argv[i], "-h")) {
			goto help;
		} else if (__strcmp(argv[i], "-o")) {
			i++;
			if (i < argc) {
				options.output = argv[i];
			} else {
				__error("You must specify a valid output file (" CYAN "-o" CRESET")");
				goto help;
			}
		} else if (__strcmp(argv[i], "-o!")) {
			i++;
			if (i < argc) {
				options.output = argv[i];
				options.force = true;
			} else {
				__error("You must specify a valid output file (" CYAN "-o!" CRESET")");
				goto help;
			}
		} else if (__strcmp(argv[i], "-c")) {
			i++;
			if (i < argc && __strcmp(argv[i], "msvc")) {
				options.compiler = COMPILER_MSVC;
			} else if (i < argc && __strcmp(argv[i], "gcc")) {
				options.compiler = COMPILER_GCC;
			} else {
				__error("You must specify a valid compiler (" CYAN "-c" CRESET").");
				__error("Allowed options are 'msvc' and 'gcc'.");
				goto help;
			}
		} else if (__strcmp(argv[i], "-l")) {
			i++;
			if (i < argc && __strcmp(argv[i], "msvc")) {
				options.linker = LINKER_MSVC;
			} else if (i < argc && __strcmp(argv[i], "crinkler")) {
				options.linker = LINKER_CRINKLER;
			} else {
				__error("You must specify a valid linker (" CYAN "-l" CRESET").");
				__error("Allowed options are 'msvc' and 'crinkler'.");
				goto help;
			}
		} else if (__strcmp(argv[i], "-x")) {
			i++;
			if (i < argc && __strcmp(argv[i], "upx")) {
				options.compression = COMPRESSION_UPX;
			} else if (i < argc && __strcmp(argv[i], "kkrunchy")) {
				options.compression = COMPRESSION_KKRUNCHY;
			} else {
				__error("You must specify a valid compressor (" CYAN "-x" CRESET").");
				__error("Allowed options are 'upx' and 'kkrunchy'.");
				goto help;
			}
		} else if (__strcmp(argv[i], "-f")) {
			i++;
			if (i < argc && __strcmp(argv[i], "size")) {
				options.tiny = true;
			} else if (i >= argc || !__strcmp(argv[i], "speed")) {
				__error("You must specify a valid optimization target (" CYAN "-f" CRESET").");
				__error("Allowed options are 'speed' and 'size'.");
				goto help;
			}
		} else if (__strcmp(argv[i], "-b")) {
			i++;
			if (i < argc && __strcmp(argv[i], "32")) {
				options.x64 = false;
			} else if (i >= argc || !__strcmp(argv[i], "64")) {
				__error("You must specify a valid bitness (" CYAN "-b" CRESET").");
				__error("Allowed options are 32 and 64.");
				goto help;
			}
		} else if (__strcmp(argv[i], "-d")) {
			options.strip = false;
		} else {
			__error("What do you mean with '%s'?", argv[i]);
			goto help;
		}
	}

	if (!options.source) {
		__error("You must specify a source file!");
		goto help;
	}
	if (!file_exists(options.source)) {
		__error("The given source file does not exist!");
		goto help;
	}

	if (file_exists(options.output) && !options.force) {
		__error("The output file already exists!");
		__error("Either change the name or force overwrite with " CYAN "-o!" CRESET);
		goto help;
	}

	__debug("Running with the following options:");
	__info(GREY "Source file: " CYAN "%s" CRESET, options.source);
	__info(GREY "Output file: " CYAN "%s (%s)" CRESET, options.output, options.force? "overwrite" : "don't overwrite");
	switch (options.compiler) {
		case COMPILER_MSVC:
			__info(GREY "   Compiler: " CYAN "MSVC (cl.exe)" CRESET);
			break;
		case COMPILER_GCC:
			__info(GREY "   Compiler: " CYAN "GCC (gcc)" CRESET);
			break;
	}
	switch (options.linker) {
		case LINKER_MSVC:
			__info(GREY "     Linker: " CYAN "MSVC (link.exe)" CRESET);
			break;
		case LINKER_CRINKLER:
			__info(GREY "     Linker: " CYAN "Crinkler" CRESET);
			break;
	}
	switch (options.compression) {
		case COMPRESSION_NONE:
			__info(GREY "Compression: " CYAN "none" CRESET);
			break;
		case COMPRESSION_UPX:
			__info(GREY "Compression: " CYAN "UPX" CRESET);
			break;
		case COMPRESSION_KKRUNCHY:
			__info(GREY "Compression: " CYAN "kkrunchy" CRESET);
			break;
	}
	__info(GREY "Focusing on: " CYAN "%s" CRESET, options.tiny? "size" : "speed");
	__info(GREY "  Stripping: " CYAN "%s" CRESET, options.strip? "yes": "no");
	__info(GREY "    Bitness: " CYAN "%d" CRESET, options.x64? 64 : 32);

	__debug("Creating build folder...");
	create_directory(".\\build.meeb");

	__start("Compiling...");
	if (options.compiler == COMPILER_MSVC) {
		__debug("Using cl.exe from MSVC");
		if (!run("cl.exe %s %s %s /Fo:.\\build.meeb\\compiled.obj",
			MSVC_COMPILE_FLAGS, options.tiny? MSVC_COMPILE_SMALL : MSVC_COMPILE_FAST, options.source)) {
			__error("Compilation failed!");
			err = 1;
			goto cleanup;
		}
	} else if (options.compiler == COMPILER_GCC) {
		__debug("Using gcc on WSL");
		if (!run("wsl.exe %s-w64-mingw32-gcc-win32 %s %s %s %s -o ./build.meeb/compiled.obj",
			options.x64? "x86_64" : "i686", options.x64? "" : "-m32", GCC_COMPILE_FLAGS, options.tiny? GCC_COMPILE_SMALL : GCC_COMPILE_FAST, options.source)) {
			__error("Compilation failed!");
			err = 1;
			goto cleanup;
		}
	} else {
		__error("Unknown compiler option (%d)", options.compiler);
		err = 1;
		goto cleanup;
	}
	__ok("Compilation finished!");
	__start("Linking...");
	if (options.linker == LINKER_MSVC) {
		__debug("Using link.exe from MSVC");
		if (!run("link.exe %s %s .\\build.meeb\\compiled.obj /OUT:.\\build.meeb\\linked.exe",
			MSVC_LINK_FLAGS, options.tiny? MSVC_LINK_SMALL : MSVC_LINK_FAST)) {
			__error("Linking failed!");
			err = 1;
			goto cleanup;
		}
	} else if (options.linker == LINKER_CRINKLER) {
		__debug("Using Crinkler");
		if (!run(".\\tools\\crinkler%d.exe %s .\\build.meeb\\compiled.obj /OUT:.\\build.meeb\\linked.exe",
			options.x64? 64 : 32, CRINKLER_LINK_FLAGS)) {
			__error("Linking failed!");
			err = 1;
			goto cleanup;
		}
	} else {
		__error("Unknown linker option (%d)", options.compiler);
		err = 1;
		goto cleanup;
	}
	__ok("Linking finished!");
	if (options.compression != COMPRESSION_NONE) {
		__start("Compressing...");
		__error("Not implemented!");
		__ok("Compression finished!");
	}

	__debug("Copying program...");
	CopyFileA(".\\build.meeb\\linked.exe", options.output, FALSE);

cleanup:
	__debug("Removing build folder...");
	remove_directory(".\\build.meeb");
	__info("Build finished.");

	free_arguments(argv);
	exit(err);

help:
	print_help();
	exit(1);
}
