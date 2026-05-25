#ifndef _SALEBLAZERS_QC_UNLOCKER_CONFIG_H_
#define _SALEBLAZERS_QC_UNLOCKER_CONFIG_H_

#include "QCUnlocker.h"

#include <Windows.h>
#include <string.h>
#include <strsafe.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct _PATCH_CONFIG {
    BOOL BypassPlacementRestrictions;
    BOOL UnlockQuantumConsoleDevMode;
    BOOL ItemGiveRarityFix;
    CHAR szLoadedConfigPath[MAX_PATH];
} PATCH_CONFIG, * PPATCH_CONFIG;

VOID LoadPatchConfig(
    _Out_ PPATCH_CONFIG pConfig
);

BOOL CreateDefaultConfig(
    _In_ LPCSTR lpszConfigFileName
);

#endif // _SALEBLAZERS_QC_UNLOCKER_CONFIG_H_