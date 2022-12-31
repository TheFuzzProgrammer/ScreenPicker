#include "screen_handler.h"

int main(int argc, unsigned short* argv[]) {

    unsigned int* file_path;

    int path_length;
   
    if (argc < 2) {
        const unsigned short s = sizeof(char) / sizeof(*argv[0]);
        printf("Not enough arguments for: %s\n", (char*)argv[0]);
        printf(" path missed?");
        return 1;
    }
    else {
        path_length = sizeof(*argv[1]) / sizeof(char);
        file_path = malloc(path_length * sizeof(unsigned int));
        for (int i = 0; i < path_length; i++) {
            file_path[i] = (unsigned int)argv[i];
        }
    }


    STARTUPINFO startupInfo;
    PROCESS_INFORMATION pi;
    HANDLE file;
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

    int iTimeSleep = 1000;
    int iSwidth = GetSystemMetrics(SM_CXSCREEN);
    int iSheight = GetSystemMetrics(SM_CYSCREEN);
    BOOL bFcreated = FALSE;

    while (TRUE) {
        HDC hdcScreen = GetDC(NULL);
        if (hdcScreen == NULL) {
            return 2;
        }
        HDC hdcBmp = CreateCompatibleDC(hdcScreen);
        if (hdcBmp == NULL) {
            return 3;
        }

        HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, iSwidth, iSheight);
        if (hBitmap == NULL) {
            return 4;
        }

        HGDIOBJ hbitmap_old = SelectObject(hdcBmp, hBitmap);
        if (hbitmap_old == NULL) {
            return 5;
        }

        if (!BitBlt(hdcBmp, 0, 0, iSwidth, iSheight, hdcScreen, 0, 0, SRCCOPY)) {
            return 6;
        }

        if (!SaveBitmap(file_path, hBitmap, &file)) {
            return 7;
        }
        SelectObject(hdcBmp, hbitmap_old);
        DeleteDC(hdcBmp);
        ReleaseDC(NULL, hdcScreen);
        DeleteObject(hBitmap);
        DeleteObject(hbitmap_old);
        Sleep(iTimeSleep);
        CloseHandle(file);
    }

    return 0;
}