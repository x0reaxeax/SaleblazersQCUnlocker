#include "Logging.h"
#include <strsafe.h>

GLOBALHANDLE g_hLogFile = INVALID_HANDLE_VALUE;

STATIC BOOL BuildLogFilePath(
    _Out_writes_(cchLogPath) LPSTR lpszLogPath,
    _In_ DWORD cchLogPath,
    _In_ LPCSTR lpszLogFileName
) {
    DWORD cchModulePath = GetModuleFileNameA(
        NULL,
        lpszLogPath,
        cchLogPath
    );

    if (0 == cchModulePath || cchModulePath >= cchLogPath) {
        return FALSE;
    }

    LPSTR lpszLastSlash = strrchr(lpszLogPath, '\\');
    if (NULL == lpszLastSlash) {
        return FALSE;
    }
    *(lpszLastSlash + 1) = '\0';

    HRESULT hResult = StringCchCatA(
        lpszLogPath,
        cchLogPath,
        lpszLogFileName
    );
    return SUCCEEDED(hResult);
}


BOOL InitializeLogFile(VOID) {

    CHAR szLogPath[MAX_PATH] = { 0 };
    if (!BuildLogFilePath(
        szLogPath, 
        _countof(szLogPath), 
        "qcunlocker.log"
    )) {
        return FALSE;
    }

    g_hLogFile = CreateFileA(
        szLogPath,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (INVALID_HANDLE_VALUE == g_hLogFile) {
        return FALSE;
    }

    return TRUE;
}

VOID LogMessage(
    _In_ LPCSTR lpFormat,
    ...
) {
    if (INVALID_HANDLE_VALUE == g_hLogFile) {
        return;
    }

    CHAR szBuffer[2048] = { 0 };
    va_list args;
    va_start(args, lpFormat);

    int nWritten = vsnprintf(
        szBuffer, 
        sizeof(szBuffer), 
        lpFormat, 
        args
    );
    va_end(args);
    if (nWritten > 0) {
        DWORD dwBytesWritten = 0;
        WriteFile(
            g_hLogFile,
            szBuffer,
            (DWORD) nWritten,
            &dwBytesWritten,
            NULL
        );
    }
}