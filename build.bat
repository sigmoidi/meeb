@echo off
cl src\meeb.c /O2 /Ot /Fo:%TEMP%\ /GS- /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB /ENTRY:main kernel32.lib ntdll.lib shell32.lib shlwapi.lib
