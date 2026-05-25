#ifndef _SALEBLAZERS_QUANTUMCONSOLE_UNLOCKER_H_
#define _SALEBLAZERS_QUANTUMCONSOLE_UNLOCKER_H_

#include "x86core.h"

#include <Windows.h>

typedef WORD UMASK, * PUMASK;
typedef CONST UMASK CUMASK, * PCUMASK; // :]

#define AOB_WC ((UMASK)0xFF00)
static_assert(AOB_WC > 0xFF, "Wildcard must not overlap byte values");

typedef struct _PatchInfo {
    LPBYTE lpTargetAddress;
    BYTE bPatchBytes[15];
    DWORD dwPatchSize;
} PATCH_INFO, *PPATCH_INFO;

#endif // _SALEBLAZERS_QUANTUMCONSOLE_UNLOCKER_H_