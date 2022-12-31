#pragma once
#include <windows.h>
#include <stdio.h>
#include <string.h>

BOOL OverrideFile(BOOL* result, HANDLE* file, LPVOID* pvBits, DWORD* dwBmpSize, DWORD* dwWritten, int tTry, PBITMAPINFO* pbmi) {
    for (int STEP = 0; STEP < tTry; STEP++) {
        *result = WriteFile(*file, *pvBits, *dwBmpSize, dwWritten, NULL);
        if (!*result || (*dwWritten != *dwBmpSize)) {
            printf("WriteFile failed %d try\n", (DWORD)tTry);
            Sleep(300);
        }
        else {
            return TRUE;
        }
        if (STEP == (tTry - 1)) {
            LocalFree(*pvBits);
            LocalFree(*pbmi);
            CloseHandle(*file);
            return FALSE;
        }
    }
}

BOOL CreateBitmapFile(HANDLE* file, LPCWSTR* file_name, int tTry, LPVOID* pvBits, PBITMAPINFO* pbmi) {
    for (int STEP = 0; STEP < tTry; STEP++) {
        *file = CreateFile((LPWSTR)*file_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS | OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE) {
            printf("CreateFile failed at try: %d\n", (DWORD)tTry);
            LocalFree(*pvBits);
            LocalFree(*pbmi);
            Sleep(300);
        }
        else {
            return TRUE;
        }
        if (STEP == (tTry - 1)) {
            return FALSE;
        }
    }
}

BOOL SaveBitmap(LPCWSTR file_name, HBITMAP bitmap, HANDLE* file) {
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    BITMAP bitmap_struct;
    PBITMAPINFO pbmi;
    WORD cClrBits;
    LPVOID pvBits;
    DWORD dwBmpSize;
    DWORD dwDIBSize;
    DWORD dwWritten;
    BOOL result;
    GetObject(bitmap, sizeof(BITMAP), (LPSTR)&bitmap_struct);
    cClrBits = (WORD)(bitmap_struct.bmPlanes * bitmap_struct.bmBitsPixel);
    if (cClrBits == 1) {
        cClrBits = 1;
    }
    else if (cClrBits <= 4) {
        cClrBits = 4;
    }
    else if (cClrBits <= 8) {
        cClrBits = 8;
    }
    else if (cClrBits <= 16) {
        cClrBits = 16;
    }
    else if (cClrBits <= 24) {
        cClrBits = 24;
    }
    else {
        cClrBits = 32;
    }
    pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits));

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = bitmap_struct.bmWidth;
    pbmi->bmiHeader.biHeight = bitmap_struct.bmHeight;
    pbmi->bmiHeader.biPlanes = bitmap_struct.bmPlanes;
    pbmi->bmiHeader.biBitCount = bitmap_struct.bmBitsPixel;
    if (cClrBits < 24) {
        pbmi->bmiHeader.biClrUsed = (1 << cClrBits);
    }
    pbmi->bmiHeader.biCompression = BI_RGB;
    dwBmpSize = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8 * pbmi->bmiHeader.biHeight;
    pvBits = (LPVOID)LocalAlloc(LPTR, dwBmpSize);

    if (!GetDIBits(CreateCompatibleDC(NULL), bitmap, 0, (WORD)pbmi->bmiHeader.biHeight, pvBits, pbmi, DIB_RGB_COLORS)) {
        LocalFree(pvBits);
        LocalFree(pbmi);
        return FALSE;
    }

    *file = CreateFile((LPWSTR)file_name, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (*file == INVALID_HANDLE_VALUE) {
        LocalFree(pvBits);
        LocalFree(pbmi);
        return FALSE;
    }

    file_header.bfType = 0x4D42;
    dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + dwBmpSize;
    file_header.bfSize = dwDIBSize;
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD);
    result = WriteFile(*file, &file_header, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);

    if (!result || (dwWritten != sizeof(BITMAPFILEHEADER))) {
        LocalFree(pvBits);
        LocalFree(pbmi);
        CloseHandle(*file);
        return FALSE;
    }

    result = WriteFile(*file, &pbmi->bmiHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
    if (!result || (dwWritten != sizeof(BITMAPINFOHEADER))) {
        LocalFree(pvBits);
        LocalFree(pbmi);
        CloseHandle(*file);
        return FALSE;
    }

    result = WriteFile(*file, &pbmi->bmiColors, pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD), &dwWritten, NULL);
    if (!result || (dwWritten != pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD))) {
        LocalFree(pvBits);
        LocalFree(pbmi);
        CloseHandle(*file);
        return FALSE;
    }

    result = WriteFile(*file, pvBits, dwBmpSize, &dwWritten, NULL);
    if (!result || (dwWritten != dwBmpSize)) {
        LocalFree(pvBits);
        LocalFree(pbmi);
        CloseHandle(*file);
        return FALSE;
    }

    LocalFree(pvBits);
    LocalFree(pbmi);

    return TRUE;
}

BOOL AsAdmin() {
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION pi;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    ZeroMemory(&pi, sizeof(pi));

    char* commandLine = "cmd.exe";
    startupInfo.lpDesktop = (LPSTR)"WinSta0\\Default";
    DWORD dwCreationFlags = CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_PROCESS_GROUP | CREATE_DEFAULT_ERROR_MODE | IDLE_PRIORITY_CLASS | CREATE_BREAKAWAY_FROM_JOB | CREATE_SUSPENDED;
    BOOL result = CreateProcessAsUser(
        NULL,
        commandLine,
        NULL,
        NULL,
        NULL,
        TRUE,
        dwCreationFlags,
        NULL,
        NULL,
        &startupInfo,
        &pi
    );

    if (result)
    {
        printf("Process ID: %d\n", pi.dwProcessId);
    }
    else
    {
        printf("Error creating process as admin.\n");
        Sleep(5000);
    }
}