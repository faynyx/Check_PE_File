#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>

void Error_193(HANDLE filehandle) {
    HANDLE hMapping = CreateFileMappingA(filehandle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping) {
        printf("Failed to map file\n");
        CloseHandle(filehandle);
        return;
    }

    LPVOID base = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!base) {
        printf("Failed to map view\n");
        CloseHandle(hMapping);
        CloseHandle(filehandle);
        return;
    }

    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        printf("Invalid DOS signature (MZ)\n");
        CloseHandle(hMapping);
        CloseHandle(filehandle);
        return;
    }

    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)((BYTE*)base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        printf("Invalid NT signature (PE)\n");
        CloseHandle(hMapping);
        CloseHandle(filehandle);
        return;
    }

    IMAGE_FILE_HEADER* fileHeader = &nt->FileHeader;
    IMAGE_OPTIONAL_HEADER* opt = &nt->OptionalHeader;

    printf("Machine: 0x%X (%s)\n", fileHeader->Machine,
        fileHeader->Machine == IMAGE_FILE_MACHINE_I386 ? "x86" :
        fileHeader->Machine == IMAGE_FILE_MACHINE_AMD64 ? "x64" : "Unknown");

    printf("Number of Sections: %d\n", fileHeader->NumberOfSections);
    printf("AddressOfEntryPoint: 0x%X\n", opt->AddressOfEntryPoint);
    printf("ImageBase: 0x%llX\n", (unsigned long long)opt->ImageBase);
    printf("SizeOfImage: 0x%X\n", opt->SizeOfImage);
    printf("Subsystem: %d\n", opt->Subsystem);

    // Entry Point Check
    if (opt->AddressOfEntryPoint > opt->SizeOfImage) {
        printf("EntryPoint > SizeOfImage Error\n");
    }

    // Section Check
    IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < fileHeader->NumberOfSections; i++) {
        printf("Section: %.8s VA: 0x%X VS: 0x%X RawSize: 0x%X\n",
            sections[i].Name,
            sections[i].VirtualAddress,
            sections[i].Misc.VirtualSize,
            sections[i].SizeOfRawData);

        if (sections[i].SizeOfRawData < sections[i].Misc.VirtualSize) {
            printf("RawDataSize < VirtualSize Error\n");
        }
    }
}

void TryCreateProcessWithDebug(LPCSTR filePath, HANDLE filehandle) {
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
    }
    else {
        DWORD err = GetLastError(); // Error Check
        LPVOID lpMsgBuf;

        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            (LPSTR)&lpMsgBuf,
            0, NULL);

        if (err == 193)
        {
            Error_193(filehandle);
        }

        printf("[FAIL ] %s (Error: %lu : %s)\n", filePath, err, (char *)lpMsgBuf); // Error Code 
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) { // Directory Search
        printf("Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", argv[1]);

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) { // File Search
        printf("No .exe files found or invalid directory: %s\n", argv[1]);
        return 1;
    }

    do { 
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // Directory Path Remove
            continue;

        printf("%s\n", searchPath);
        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s\\%s", argv[1], findFileData.cFileName);
        TryCreateProcessWithDebug(fullPath, hFind);
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
    return 0;
}
