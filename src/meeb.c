#include <windows.h>
#include <inttypes.h>
#undef small // sigh... https://stackoverflow.com/a/27794577

#define true 1
#define false 0
typedef uint_fast8_t bool;

#define DEBUG false
#define VERSION "1.0"

#include "utils.h"

enum CompilerOpt { COMPILER_MSVC, COMPILER_GCC };
enum LinkerOpt { LINKER_MSVC, LINKER_CRINKLER };
enum CompressionOpt { COMPRESSION_NONE, COMPRESSION_UPX, COMPRESSION_KKRUNCHY };

#define BUILD_DIR "build.meeb"

#define MSVC_COMPILE_FLAGS "/nologo /GS- /c"
#define MSVC_LINK_FLAGS "/nologo /SUBSYSTEM:CONSOLE /NODEFAULTLIB /ENTRY:entry kernel32.lib ntdll.lib shell32.lib shlwapi.lib user32.lib"

#define MSVC_COMPILE_FAST "/O2 /Ot"
#define MSVC_COMPILE_SMALL "/O1 /Os"
#define MSVC_LINK_FAST ""
#define MSVC_LINK_SMALL "/ALIGN:16 /FILEALIGN:16"

#define GCC_COMPILE_FLAGS "-std=c17 -nostdlib -nostartfiles -c"

#define GCC_COMPILE_FAST "-O3"
#define GCC_COMPILE_SMALL "-Os"

#define CRINKLER_LINK_FLAGS "/CRINKLER /COMPMODE:VERYSLOW /SUBSYSTEM:CONSOLE /NODEFAULTLIB /ENTRY:entry kernel32.lib ntdll.lib shell32.lib shlwapi.lib user32.lib"

int main() {
	int err = 0;

	int argc;
	char** argv;

	load_libraries();
	get_arguments(&argc, &argv);

	if (argc < 2) {
		print_help();
		exit(1);
	}

	struct {
		char* source;
		char* output;
		bool force;
		enum CompilerOpt compiler;
		enum LinkerOpt linker;
		enum CompressionOpt compression;
		bool strip;
		bool x64;
		bool small;
	} options = {0, "program.exe", false, COMPILER_MSVC, LINKER_MSVC, COMPRESSION_NONE, true, true, false};

	for (int i = 1; i < argc; i++) {
		if (!options.source && (__strlen(argv[i]) > 2 || argv[i][0] != '-')) {
			options.source = argv[i];
		} else if (__strcmp(argv[i], "-h")) {
			print_help();
			exit(1);
		} else if (__strcmp(argv[i], "-o")) {
			i++;
			if (i < argc) {
				options.output = argv[i];
			} else {
				__error("You must specify a valid output file (" CYAN "-o" CRESET")");
				print_options();
				exit(1);
			}
		} else if (__strcmp(argv[i], "-o!")) {
			i++;
			if (i < argc) {
				options.output = argv[i];
				options.force = true;
			} else {
				__error("You must specify a valid output file (" CYAN "-o!" CRESET")");
				print_options();
				exit(1);
			}
		} else if (__strcmp(argv[i], "-c")) {
			i++;
			if (i < argc && __strcmp(argv[i], "msvc")) {
				options.compiler = COMPILER_MSVC;
			} else if (i < argc && __strcmp(argv[i], "gcc")) {
				options.compiler = COMPILER_GCC;
			} else {
				__error("You must specify a valid compiler (" CYAN "-c" CRESET").");
				print_options();
				exit(1);
			}
		} else if (__strcmp(argv[i], "-l")) {
			i++;
			if (i < argc && __strcmp(argv[i], "msvc")) {
				options.linker = LINKER_MSVC;
			} else if (i < argc && __strcmp(argv[i], "crinkler")) {
				options.linker = LINKER_CRINKLER;
			} else {
				__error("You must specify a valid linker (" CYAN "-l" CRESET").");
				print_options();
				exit(1);
			}
		} else if (__strcmp(argv[i], "-x")) {
			i++;
			if (i < argc && __strcmp(argv[i], "upx")) {
				options.compression = COMPRESSION_UPX;
			} else if (i < argc && __strcmp(argv[i], "kkrunchy")) {
				options.compression = COMPRESSION_KKRUNCHY;
			} else {
				__error("You must specify a valid compressor (" CYAN "-x" CRESET").");
				print_options();
				exit(1);
			}
		} else if (__strcmp(argv[i], "-f")) {
			i++;
			if (i < argc && __strcmp(argv[i], "small")) {
				options.small = true;
			} else if (i >= argc || !__strcmp(argv[i], "fast")) {
				__error("You must specify a valid optimization goal (" CYAN "-f" CRESET").");
				print_options();
				exit(1);
			}
		} else if (__strcmp(argv[i], "-b")) {
			i++;
			if (i < argc && __strcmp(argv[i], "32")) {
				options.x64 = false;
			} else if (i >= argc || !__strcmp(argv[i], "64")) {
				__error("You must specify a valid bitness (" CYAN "-b" CRESET").");
				print_options();
				exit(1);
			}
		} else if (__strcmp(argv[i], "-d")) {
			options.strip = false;
		} else {
			__error("What do you mean with '%s'?", argv[i]);
			print_options();
			exit(1);
		}
	}

	if (!options.source) {
		__error("You must specify a source file!");
		print_options();
		exit(1);
	}
	if (!file_exists(options.source)) {
		__error("The given source file does not exist!");
		print_options();
		exit(1);
	}

	if (file_exists(options.output) && !options.force) {
		__error("The output file already exists!");
		__error("Either change the name or force overwrite with " CYAN "-o!" CRESET);
		print_options();
		exit(1);
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
	__info(GREY "Focusing on: " CYAN "%s" CRESET, options.small? "size" : "speed");
	__info(GREY "  Stripping: " CYAN "%s" CRESET, options.strip? "yes": "no");
	__info(GREY "    Bitness: " CYAN "%d" CRESET, options.x64? 64 : 32);

	__debug("Creating build folder...");
	create_directory(".\\" BUILD_DIR);

	__start("Compiling...");
	if (options.compiler == COMPILER_MSVC) {
		__debug("Using cl.exe from MSVC");
		if (!run("cl.exe " MSVC_COMPILE_FLAGS " %s %s /Fo:.\\" BUILD_DIR "\\compiled.obj",
			options.small? MSVC_COMPILE_SMALL : MSVC_COMPILE_FAST, options.source)) {
			__error("Compilation failed!");
			err = 1;
			goto cleanup;
		}
	} else if (options.compiler == COMPILER_GCC) {
		__debug("Using gcc on WSL");
		if (!run("wsl.exe x86_64-w64-mingw32-gcc-win32 " GCC_COMPILE_FLAGS " %s %s %s -o ./" BUILD_DIR "/compiled.obj",
			options.x64? "" : "-m32", options.small? GCC_COMPILE_SMALL : GCC_COMPILE_FAST, options.source)) {
			__error("Compilation failed!");
			err = 1;
			goto cleanup;
		}
	}
	__ok("Compilation finished!");
	__start("Linking...");
	if (options.linker == LINKER_MSVC) {
		__debug("Using link.exe from MSVC");
		if (!run("link.exe " MSVC_LINK_FLAGS " %s .\\" BUILD_DIR "\\compiled.obj /OUT:.\\" BUILD_DIR "\\linked.exe",
			options.small? MSVC_LINK_SMALL : MSVC_LINK_FAST)) {
			__error("Linking failed!");
			err = 1;
			goto cleanup;
		}
	} else if (options.linker == LINKER_CRINKLER) {
		__debug("Using Crinkler");
		if (!run(".\\tools\\crinkler.exe " CRINKLER_LINK_FLAGS " .\\" BUILD_DIR "\\compiled.obj /OUT:.\\" BUILD_DIR "\\linked.exe")) {
			__error("Linking failed!");
			err = 1;
			goto cleanup;
		}
	}
	__ok("Linking finished!");
	if (options.compression != COMPRESSION_NONE) {
		__start("Compressing...");

		if (options.compression == COMPRESSION_UPX) {
			__debug("Using UPX");
			if (!run(".\\tools\\upx.exe -9 .\\" BUILD_DIR "\\linked.exe -o .\\" BUILD_DIR "\\compressed.exe")) {
				__error("Compression failed!");
				err = 1;
				goto cleanup;
			}
		} else if (options.compression == COMPRESSION_KKRUNCHY) {
			__debug("Using Kkrunchy");
			if (!run(".\\tools\\kkrunchy.exe --best .\\" BUILD_DIR "\\linked.exe --out .\\" BUILD_DIR "\\compressed.exe")) {
				__error("Compression failed!");
				err = 1;
				goto cleanup;
			}
		}

		__debug("Copying program...");
		if (!CopyFileA(".\\" BUILD_DIR "\\compressed.exe", options.output, FALSE)) {
			__error("Could not copy compressed program!");
			__error("Check the output from the tools above.");
			goto cleanup;
		}

		__ok("Compression finished!");
	} else {
		__debug("Copying program...");
		CopyFileA(".\\" BUILD_DIR "\\linked.exe", options.output, FALSE);
	}

cleanup:
	__debug("Removing build folder...");
	remove_directory(".\\" BUILD_DIR);
	__info("Build finished.");

	free_arguments(argv);
	exit(err);
}
