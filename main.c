#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>

void TryCreateProcessWithDebug(LPCSTR filePath) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL success = CreateProcessA(
        filePath, NULL, NULL, NULL, FALSE,
        DEBUG_PROCESS | CREATE_NEW_CONSOLE,
        NULL, NULL, &si, &pi
    );

    if (success) {
        printf("[OK   ] %s\n", filePath);
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        DWORD err = GetLastError();
        printf("[FAIL ] %s (Error: %lu)\n", filePath, err);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*.exe", argv[1]);

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No .exe files found or invalid directory: %s\n", argv[1]);
        return 1;
    }

    do {
        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s\\%s", argv[1], findFileData.cFileName);
        TryCreateProcessWithDebug(fullPath);
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
    return 0;
}
