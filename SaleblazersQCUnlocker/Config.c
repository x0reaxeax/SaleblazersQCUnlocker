#include "Config.h"
#include "Logging.h"

STATIC VOID InitializeDefaultPatchConfig(
    _Out_ PPATCH_CONFIG pConfig
) {
    ZeroMemory(pConfig, sizeof(*pConfig));

    pConfig->BypassPlacementRestrictions = FALSE;
    pConfig->UnlockQuantumConsoleDevMode = TRUE;
    pConfig->ItemGiveRarityFix = TRUE;
}

STATIC LPSTR TrimWhitespace(
    _Inout_ LPSTR lpszText
) {
    if (NULL == lpszText) {
        return NULL;
    }

    while ('\0' != *lpszText && isspace((unsigned char) *lpszText)) {
        lpszText++;
    }

    LPSTR lpszEnd = lpszText + strlen(lpszText);
    while (lpszEnd > lpszText && isspace((unsigned char) *(lpszEnd - 1))) {
        lpszEnd--;
    }
    *lpszEnd = '\0';

    return lpszText;
}

#pragma warning(push)
#pragma warning(disable: 6101)  // returning uninitialized memory for pbResult - intentional 
STATIC BOOL TryParseBoolean(
    _In_ LPCSTR lpszValue,
    _Out_opt_ PBOOL pbResult
) {
    if (NULL == lpszValue || NULL == pbResult) {
        return FALSE;
    }

    if (
        0 == _stricmp(lpszValue, "1") ||
        0 == _stricmp(lpszValue, "true")
    ) {
        *pbResult = TRUE;
        return TRUE;
    }

    if (
        0 == _stricmp(lpszValue, "0") ||
        0 == _stricmp(lpszValue, "false")
    ) {
        *pbResult = FALSE;
        return TRUE;
    }

    return FALSE;
}
#pragma warning(pop)

STATIC LPCSTR FindStringInsensitive(
    _In_ LPCSTR lpszHaystack,
    _In_ LPCSTR lpszNeedle
) {
    SIZE_T cchNeedle = strlen(lpszNeedle);

    if (0 == cchNeedle) {
        return lpszHaystack;
    }

    for (LPCSTR lpszCurrent = lpszHaystack; '\0' != *lpszCurrent; lpszCurrent++) {
        if (0 == _strnicmp(lpszCurrent, lpszNeedle, cchNeedle)) {
            return lpszCurrent;
        }
    }

    return NULL;
}

STATIC BOOL ApplyPatchConfigValue(
    _Inout_ PPATCH_CONFIG pConfig,
    _In_ LPCSTR lpszKey,
    _In_ LPCSTR lpszValue
) {
    BOOL bValue = FALSE;
    if (
        NULL == pConfig ||
        NULL == lpszKey ||
        NULL == lpszValue ||
        !TryParseBoolean(lpszValue, &bValue)
        ) {
        return FALSE;
    }

    if (0 == _stricmp(lpszKey, "BypassPlacementRestrictions")) {
        pConfig->BypassPlacementRestrictions = bValue;
        return TRUE;
    }

    if (0 == _stricmp(lpszKey, "UnlockQuantumConsoleDevMode")) {
        pConfig->UnlockQuantumConsoleDevMode = bValue;
        return TRUE;
    }

    if (0 == _stricmp(lpszKey, "ItemGiveRarityFix")) {
        pConfig->ItemGiveRarityFix = bValue;
        return TRUE;
    }

    return FALSE;
}

STATIC VOID ParseKeyValueConfig(
    _Inout_ PPATCH_CONFIG pConfig,
    _Inout_ LPSTR lpszConfigText
) {
    LPSTR lpszContext = NULL;
    LPSTR lpszLine = strtok_s(lpszConfigText, "\r\n", &lpszContext);

    while (NULL != lpszLine) {
        LPSTR lpszTrimmedLine = TrimWhitespace(lpszLine);

        if (
            '\0' != *lpszTrimmedLine &&
            '#' != *lpszTrimmedLine &&
            ';' != *lpszTrimmedLine &&
            '[' != *lpszTrimmedLine
        ) {
            LPSTR lpszEquals = strchr(lpszTrimmedLine, '=');
            if (NULL != lpszEquals) {
                *lpszEquals = '\0';

                LPSTR lpszKey = TrimWhitespace(lpszTrimmedLine);
                LPSTR lpszValue = TrimWhitespace(lpszEquals + 1);

                LPSTR lpszComment = strpbrk(lpszValue, "#;");
                if (NULL != lpszComment) {
                    *lpszComment = '\0';
                    lpszValue = TrimWhitespace(lpszValue);
                }

                ApplyPatchConfigValue(pConfig, lpszKey, lpszValue);
            }
        }

        lpszLine = strtok_s(NULL, "\r\n", &lpszContext);
    }
}

STATIC VOID ParseJsonConfigValue(
    _Inout_ PPATCH_CONFIG pConfig,
    _In_ LPCSTR lpszConfigText,
    _In_ LPCSTR lpszKey
) {
    CHAR szNeedle[64] = { 0 };
    _snprintf_s(szNeedle, sizeof(szNeedle), _TRUNCATE, "\"%s\"", lpszKey);

    LPCSTR lpszKeyHit = FindStringInsensitive(lpszConfigText, szNeedle);
    if (NULL == lpszKeyHit) {
        return;
    }

    LPCSTR lpszColon = strchr(lpszKeyHit + strlen(szNeedle), ':');
    if (NULL == lpszColon) {
        return;
    }

    CHAR szValue[16] = { 0 };
    LPCSTR lpszValueStart = lpszColon + 1;
    while ('\0' != *lpszValueStart && isspace((unsigned char) *lpszValueStart)) {
        lpszValueStart++;
    }

    if ('"' == *lpszValueStart) {
        lpszValueStart++;
    }

    SIZE_T cchValue = 0;
    while (
        '\0' != lpszValueStart[cchValue] &&
        '"' != lpszValueStart[cchValue] &&
        ',' != lpszValueStart[cchValue] &&
        '}' != lpszValueStart[cchValue] &&
        !isspace((unsigned char) lpszValueStart[cchValue]) &&
        cchValue < sizeof(szValue) - 1
    ) {
        szValue[cchValue] = lpszValueStart[cchValue];
        cchValue++;
    }

    szValue[cchValue] = '\0';
    ApplyPatchConfigValue(pConfig, lpszKey, szValue);
}

STATIC VOID ParseJsonConfig(
    _Inout_ PPATCH_CONFIG pConfig,
    _In_ LPCSTR lpszConfigText
) {
    ParseJsonConfigValue(pConfig, lpszConfigText, "BypassPlacementRestrictions");
    ParseJsonConfigValue(pConfig, lpszConfigText, "UnlockQuantumConsoleDevMode");
    ParseJsonConfigValue(pConfig, lpszConfigText, "ItemGiveRarityFix");
}

STATIC BOOL BuildConfigPath(
    _Out_writes_(cchConfigPath) LPSTR lpszConfigPath,
    _In_ DWORD cchConfigPath,
    _In_ LPCSTR lpszConfigFileName
) {
    DWORD cchModulePath = GetModuleFileNameA(
        NULL,
        lpszConfigPath,
        cchConfigPath
    );

    if (0 == cchModulePath || cchModulePath >= cchConfigPath) {
        return FALSE;
    }

    LPSTR lpszLastSlash = strrchr(lpszConfigPath, '\\');
    if (NULL == lpszLastSlash) {
        return FALSE;
    }

    *(lpszLastSlash + 1) = '\0';

    HRESULT hResult = StringCchCatA(
        lpszConfigPath,
        cchConfigPath,
        lpszConfigFileName
    );

    return SUCCEEDED(hResult);
}

STATIC BOOL LoadPatchConfigFile(
    _Inout_ PPATCH_CONFIG pConfig,
    _In_ LPCSTR lpszConfigFileName,
    _In_ BOOL bJson
) {
    CHAR szConfigPath[MAX_PATH] = { 0 };
    if (!BuildConfigPath(szConfigPath, _countof(szConfigPath), lpszConfigFileName)) {
        return FALSE;
    }

    HANDLE hConfigFile = CreateFileA(
        szConfigPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (INVALID_HANDLE_VALUE == hConfigFile) {
        return FALSE;
    }

    DWORD cbFileSize = GetFileSize(hConfigFile, NULL);
    if (INVALID_FILE_SIZE == cbFileSize || 0 == cbFileSize || cbFileSize > 64 * 1024) {
        CloseHandle(hConfigFile);
        return FALSE;
    }

    LPSTR lpszConfigText = (LPSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T) cbFileSize + 1);
    if (NULL == lpszConfigText) {
        CloseHandle(hConfigFile);
        return FALSE;
    }

    DWORD cbRead = 0;
    BOOL bRead = ReadFile(hConfigFile, lpszConfigText, cbFileSize, &cbRead, NULL);
    CloseHandle(hConfigFile);

    if (bRead && cbRead > 0) {
        lpszConfigText[cbRead] = '\0';

        if (bJson) {
            ParseJsonConfig(pConfig, lpszConfigText);
        } else {
            ParseKeyValueConfig(pConfig, lpszConfigText);
        }

        StringCchCopyA(pConfig->szLoadedConfigPath, _countof(pConfig->szLoadedConfigPath), szConfigPath);
    }

    HeapFree(GetProcessHeap(), 0, lpszConfigText);
    return bRead && cbRead > 0;
}

VOID LoadPatchConfig(
    _Out_ PPATCH_CONFIG pConfig
) {
    InitializeDefaultPatchConfig(pConfig);

    if (LoadPatchConfigFile(pConfig, "SaleblazersQCUnlocker.ini", FALSE)) {
        return;
    }

    if (LoadPatchConfigFile(pConfig, "SaleblazersQCUnlocker.cfg", FALSE)) {
        return;
    }

    LoadPatchConfigFile(pConfig, "SaleblazersQCUnlocker.json", TRUE);
}

BOOL CreateDefaultConfig(
    _In_ LPCSTR lpszConfigFileName
) {
    CHAR szConfigPath[MAX_PATH] = { 0 };
    if (!BuildConfigPath(szConfigPath, _countof(szConfigPath), lpszConfigFileName)) {
        return FALSE;
    }
    HANDLE hConfigFile = CreateFileA(
        szConfigPath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (INVALID_HANDLE_VALUE == hConfigFile) {
        return FALSE;
    }
    const CHAR* cszDefaultConfigContent =
        "# Saleblazers QuantumConsole Unlocker default config\n"
        "\n"
        "# Bypass placement restrictions (default: false)\n"
        "BypassPlacementRestrictions=false\n"
        "\n"
        "# Unlock Quantum Console dev mode (default: true)\n"
        "UnlockQuantumConsoleDevMode=true\n"
        "\n"
        "# Fix item give rarity bug (default: true)\n"
        "ItemGiveRarityFix=true\n";
    DWORD dwBytesWritten = 0;
    BOOL bWrite = WriteFile(
        hConfigFile,
        cszDefaultConfigContent,
        (DWORD) strlen(cszDefaultConfigContent),
        &dwBytesWritten,
        NULL
    );
    CloseHandle(hConfigFile);
    return bWrite && dwBytesWritten == strlen(cszDefaultConfigContent);
}
