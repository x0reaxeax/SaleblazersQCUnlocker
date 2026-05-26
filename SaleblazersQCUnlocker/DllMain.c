// Saleblazers QCUnlocker - Game Version v0.21.0.2
//  - https://github.com/x0reaxeax/SaleblazersQCUnlocker
// 
// 
// Free Move
//  * BaseItemPlacingManager.CanPlaceObject
//      RVA: 0x121BD00 Offset: 0x121AF00 VA: 0x18121BD00
//	        public bool CanPlaceObject() { } 
// 
// QuantumConsole Patcher
//  * QuantumConsole.IsValidCommand
//      :: Base - GameAssembly.dll
//          RVA @ 0x04D57ED0
//              public static bool IsValidCommand(CommandData command) { }
//      :: Patch
//          B8 01 00 00 00 C3                    ; MOV EAX, 1; RETN
// 
//  * HRConsoleCommands.CheckAllowDeveloperCommand
//      :: Base - GameAssembly.dll
//          RVA @ 0x0166CE30
//              public static bool CheckAllowDeveloperCommand(bool HostOnlyCheat = False, bool IsCheat = True) { }
//      :: Patch
//          B8 01 00 00 00 C3                    ; MOV EAX, 1; RETN
//
// Inline Developer Gate ("DEV" check)
//     :: Signature
//         48 8B 11                ; MOV RDX, [RCX]
//         48 8B 82 E8 01 00 00    ; MOV RAX, [RDX+0x1E8]
//         48 8B 92 F0 01 00 00    ; MOV RDX, [RDX+0x1F0]
//         FF D0                   ; CALL RAX
//         84 C0                   ; TEST AL, AL
//         0F 84 ?? ?? ?? ??       ; JZ <GameAssembly.dll + ???>
//         48 8B 0D ?? ?? ?? ??    ; MOV RCX, cs:<???_TypeInfo>
//     :: Patch ( +0x15 from sig start )
//         66 0F 1F 44 00 00       ; NOP WORD PTR [RAX + RAX + 00h]
//     ** Unlocks:
//        - give_item_name
//        - give_item
//        - spawn_item
//        - spawn_item
//        - UnlockRecipeGlobal
//        - UnlockRecipeLocal
//    
//     :: Signature
//         48 8B 11                ; MOV RDX, [RCX]
//         48 8B 82 E8 01 00 00    ; MOV RAX, [RDX+0x1E8]
//         48 8B 92 F0 01 00 00    ; MOV RDX, [RDX+0x1F0]
//         FF D0                   ; CALL RAX
//         84 C0                   ; TEST AL, AL
//         0F 84 ?? ?? ?? ??       ; JZ <GameAssembly.dll + ???>
//         48 8B 05 ?? ?? ?? ??    ; MOV RAX, cs:<BaseGameInstance_TypeInfo>
//     :: Patch ( +0x15 from sig start )
//         66 0F 1F 44 00 00       ; NOP WORD PTR [RAX + RAX + 00h]
//
// Give Commands Rarity Fix
//     RVA:      0x0167D9D4
//     Original: 45 33 C9        ; XOR R9D, R9D
//     Patch:    41 B1 01        ; MOV R9B, 1       ; bypassLimit=true


#include <Windows.h>
#include <stdio.h>

#include "QCUnlocker.h"
#include "Logging.h"
#include "Config.h"

GLOBALHANDLE g_hThread = NULL;
DWORD g_dwThreadId = 0;

LPCBYTE SearchForMaskedSignature(
    _In_reads_bytes_(SearchSize) LPCVOID SearchBase,
    _In_ SIZE_T SearchSize,
    _In_reads_(SignatureCount) PCUMASK Signature,
    _In_ SIZE_T SignatureCount
) {
    if (NULL == SearchBase || NULL == Signature || SignatureCount == 0) {
        return NULL;
    }

    if (SearchSize < SignatureCount) {
        return NULL;
    }

    LPCBYTE lpBytes = (LPCBYTE) SearchBase;
    CONST SIZE_T cStartCount = SearchSize - SignatureCount + 1;

    for (SIZE_T i = 0; i < cStartCount; ++i) {
        LPCBYTE lpWindow = lpBytes + i;
        BOOLEAN bMatch = TRUE;

        for (SIZE_T j = 0; j < SignatureCount; ++j) {
            UMASK wToken = Signature[j];

            if (AOB_WC != wToken && lpWindow[j] != (BYTE) wToken) {
                bMatch = FALSE;
                break;
            }
        }

        if (bMatch) {
            return lpWindow;
        }
    }
    return NULL;
}

SIZE_T SearchForMaskedSignatureAll(
    _In_reads_bytes_(SearchSize) LPCVOID SearchBase,
    _In_ SIZE_T SearchSize,
    _In_reads_(SignatureCount) PCUMASK Signature,
    _In_ SIZE_T SignatureCount,
    _Out_writes_to_(MaxMatches, return) LPCBYTE* Matches,
    _In_ SIZE_T MaxMatches
) {
    if (
        NULL == SearchBase ||
        NULL == Signature ||
        NULL == Matches ||
        0 == SearchSize ||
        0 == SignatureCount ||
        0 == MaxMatches
    ) {
        return 0;
    }

    if (SearchSize < SignatureCount) {
        return 0;
    }

    SIZE_T cMatches = 0;
    LPCBYTE lpCurrentBase = (LPCBYTE) SearchBase;
    SIZE_T cbRemaining = SearchSize;

    while (cMatches < MaxMatches && cbRemaining >= SignatureCount) {
        LPCBYTE lpHit = SearchForMaskedSignature(
            lpCurrentBase,
            cbRemaining,
            Signature,
            SignatureCount
        );

        if (NULL == lpHit) {
            break;
        }

        Matches[cMatches++] = lpHit;

        // no overlap, just move past current hit, deal with it, get over it
        SIZE_T cbConsumed = (SIZE_T) ((lpHit - lpCurrentBase) + SignatureCount);

        lpCurrentBase += cbConsumed;
        cbRemaining -= cbConsumed;
    }

    return cMatches;
}

SIZE_T GetModuleSize(
    _In_ HMODULE hModule
) {
    if (NULL == hModule) {
        return 0;
    }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) hModule;
    if (IMAGE_DOS_SIGNATURE != pDosHeader->e_magic) {
        return 0;
    }

    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS) ((LPBYTE) hModule + pDosHeader->e_lfanew);
    if (IMAGE_NT_SIGNATURE != pNtHeaders->Signature) {
        return 0;
    }

    return pNtHeaders->OptionalHeader.SizeOfImage;
}

BOOL WriteExecMemory(
    _In_ LPBYTE lpAddress,
    _In_reads_bytes_(Size) LPCVOID lpBuffer,
    _In_ SIZE_T Size
) {
    if (NULL == lpAddress || NULL == lpBuffer || Size == 0) {
        return FALSE;
    }

    DWORD dwOldProtect = 0;
    if (!VirtualProtect(
        lpAddress,
        Size,
        PAGE_EXECUTE_READWRITE,
        &dwOldProtect
    )) {
        LogMessage(
            "VirtualProtect() failed for address %p - E%lu",
            lpAddress,
            GetLastError()
        );
        return FALSE;
    }

    memcpy(
        lpAddress,
        lpBuffer,
        Size
    );

    DWORD dwTemp;
    if (!VirtualProtect(
        lpAddress,
        Size,
        dwOldProtect,
        &dwTemp
    )) {
        LogMessage(
            "Failed to revert memory protection for address %p - E%lu",
            lpAddress,
            GetLastError()
        );
    }
    return TRUE;
}

BOOL PatchPlacementRestrictions(
    _In_ LPCBYTE lpGameAssemblyBase
) {
    if (NULL == lpGameAssemblyBase) {
        return FALSE;
    }

    LPBYTE lpTargetPatchAddress = (LPBYTE) lpGameAssemblyBase + 0x121BD00;
    BYTE abPatchBytes[6] = { 
        0xB8, 0x01, 0x00, 0x00, 0x00,   // MOV EAX, 1   // just in case the boolean return is greater than 1 byte
        0xC3                            // RET
    };

    LogMessage("Patch size for placement restriction bypass: %llu bytes\n", sizeof(abPatchBytes));

    return WriteExecMemory(
        lpTargetPatchAddress,
        abPatchBytes,
        sizeof(abPatchBytes)
    );
}

BOOL PatchQuantumConsoleDevMode(
    _In_ LPCBYTE lpGameAssemblyBase
) {
    if (NULL == lpGameAssemblyBase) {
        return FALSE;
    }
    
    CONST PATCH_INFO aQuantumConsoleProloguePatches[] = {
        {
            (LPBYTE) lpGameAssemblyBase + 0x04D57ED0,       // QuantumConsole.IsValidCommand
            { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 },
            6
        },
        {
            (LPBYTE) lpGameAssemblyBase + 0x0166CE30,       // HRConsoleCommands.CheckAllowDeveloperCommand
            { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 },
            6
        }
    };

    LogMessage("Number of QuantumConsole prologue patches: %llu\n", ARRAYSIZE(aQuantumConsoleProloguePatches));

    for (DWORD i = 0; i < ARRAYSIZE(aQuantumConsoleProloguePatches); i++) {
        if (!WriteExecMemory(
            aQuantumConsoleProloguePatches[i].lpTargetAddress,
            aQuantumConsoleProloguePatches[i].bPatchBytes,
            aQuantumConsoleProloguePatches[i].dwPatchSize
        )) {
            LogMessage(
                "Failed to apply Quantum Console prologue patch %lu/%lu @ %p\n",
                i + 1,
                ARRAYSIZE(aQuantumConsoleProloguePatches),
                aQuantumConsoleProloguePatches[i].lpTargetAddress
            );
            return FALSE;
        }

        LogMessage(
            "Prologue patch %lu/%lu applied @ %p\n",
            i + 1,
            ARRAYSIZE(aQuantumConsoleProloguePatches),
            aQuantumConsoleProloguePatches[i].lpTargetAddress
        );
    }

    LogMessage("Quantum Console dev mode patches applied successfully.\n");

    return TRUE;
}

BOOL PatchQCInlineDevGates(
    _In_ LPCBYTE lpGameAssemblyBase,
    _In_reads_(cDevGateSignature) PCUMASK pwDevGateSignature,
    _In_ SIZE_T cDevGateSignature,
    _In_reads_(cTargetAddresses) LPBYTE* alpTargetAddresses,
    _In_ SIZE_T cTargetAddresses
) {
    if (NULL == lpGameAssemblyBase) {
        return FALSE;
    }

    if (
        NULL == pwDevGateSignature || cDevGateSignature == 0 || 
        NULL == alpTargetAddresses || cTargetAddresses == 0
    ) {
        return FALSE;
    }

    CONST BYTE abInlineDevGatePatch[6] = {
        0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00
    };
    // the JZ is patched
    CONST SIZE_T cbInlineDevGatePatchOffset = 0x15;

    SIZE_T cAppliedInlinePatches = 0;
    for (DWORD i = 0; i < cTargetAddresses; i++) {
        LPBYTE lpPatchAddress = alpTargetAddresses[i] + cbInlineDevGatePatchOffset;
        
        if (!WriteExecMemory(
            lpPatchAddress,
            abInlineDevGatePatch,
            sizeof(abInlineDevGatePatch)
        )) {
            LogMessage(
                "Failed to apply inline dev gate patch %lu/%lu @ %p\n",
                i + 1,
                cTargetAddresses,
                lpPatchAddress
            );
            continue;
        }

        LogMessage(
            "Inline dev gate patch %lu/%lu applied @ %p\n",
            i + 1,
            cTargetAddresses,
            lpPatchAddress
        );

        cAppliedInlinePatches++;
    }
    LogMessage("Applied %llu inline dev gate patches.\n", cAppliedInlinePatches);

    return TRUE;
}

BOOL ApplyQCDevPatches(
    _In_ LPCBYTE lpGameAssemblyBase,
    _In_ SIZE_T cbGameAssemblySize
) {
    if (NULL == lpGameAssemblyBase || 0 == cbGameAssemblySize) {
        return FALSE;
    }

    CONST UMASK awInlineDevGateSignatureCheatCommands[] = {
        0x48, 0x8B, 0x11,                                   // MOV RDX, [RCX]
        0x48, 0x8B, 0x82, 0xE8, 0x01, 0x00, 0x00,           // MOV RAX, [RDX+0x1E8]
        0x48, 0x8B, 0x92, 0xF0, 0x01, 0x00, 0x00,           // MOV RDX, [RDX+0x1F0]
        0xFF, 0xD0,                                         // CALL RAX
        0x84, 0xC0,                                         // TEST AL, AL
        0x0F, 0x84, AOB_WC, AOB_WC, AOB_WC, AOB_WC,         // JZ <GameAssembly.dll + ???>
        0x48, 0x8B, 0x0D                                    // MOV RCX, cs:<???_TypeInfo>
    };

    CONST UMASK awInlineDevGateSignatureDebugHelpers[] = {
        0x48, 0x8B, 0x11,                                   // MOV RDX, [RCX]
        0x48, 0x8B, 0x82, 0xE8, 0x01, 0x00, 0x00,           // MOV RAX, [RDX+0x1E8]
        0x48, 0x8B, 0x92, 0xF0, 0x01, 0x00, 0x00,           // MOV RDX, [RDX+0x1F0]
        0xFF, 0xD0,                                         // CALL RAX
        0x84, 0xC0,                                         // TEST AL, AL
        0x0F, 0x84, AOB_WC, AOB_WC, AOB_WC, AOB_WC,         // JZ <GameAssembly.dll + ???>
        0x48, 0x8B, 0x05                                    // MOV RAX, cs:<BaseGameInstance_TypeInfo>
    };

    // should be 6 of these
    LPBYTE alpInlineDevGatedCheatCommandsHits[6] = { NULL };
    SIZE_T cCheatCommandsInlineGates = SearchForMaskedSignatureAll(
        lpGameAssemblyBase,
        cbGameAssemblySize,
        awInlineDevGateSignatureCheatCommands,
        _countof(awInlineDevGateSignatureCheatCommands),
        alpInlineDevGatedCheatCommandsHits,
        _countof(alpInlineDevGatedCheatCommandsHits)
    );

    if (0 == cCheatCommandsInlineGates) {
        LogMessage("Failed to find any hits for inline dev gates for console commands signature\n");
        return FALSE;
    }

    // should be 3 of these
    LPBYTE alpInlineDevGatedDebugHelpersHits[3] = { NULL };
    SIZE_T cDebugHelpersHits = SearchForMaskedSignatureAll(
        lpGameAssemblyBase,
        cbGameAssemblySize,
        awInlineDevGateSignatureDebugHelpers,
        _countof(awInlineDevGateSignatureDebugHelpers),
        alpInlineDevGatedDebugHelpersHits,
        _countof(alpInlineDevGatedDebugHelpersHits)
    );

    if (0 == cDebugHelpersHits) {
        LogMessage("Failed to find any hits for inline dev gates for debug helpers signature\n");
        return FALSE;
    }

    LogMessage("Found %llu hit(s) for inline dev cheat command gates \n", cCheatCommandsInlineGates);
    LogMessage("Found %llu hit(s) for inline dev gates for debug helpers signature\n", cDebugHelpersHits);

    if (!PatchQCInlineDevGates(
        lpGameAssemblyBase,
        awInlineDevGateSignatureDebugHelpers,
        _countof(awInlineDevGateSignatureDebugHelpers),
        alpInlineDevGatedCheatCommandsHits,
        cCheatCommandsInlineGates
    )) {
        LogMessage("Failed to patch inline dev gates for debug helpers\n");
        return FALSE;
    }

    if (!PatchQCInlineDevGates(
        lpGameAssemblyBase,
        awInlineDevGateSignatureDebugHelpers,
        _countof(awInlineDevGateSignatureDebugHelpers),
        alpInlineDevGatedDebugHelpersHits,
        cDebugHelpersHits
    )) {
        LogMessage("Failed to patch inline dev gates for console commands\n");
        return FALSE;
    }

    return TRUE;
}

BOOL PatchItemGiveRarityBug(
    _In_ LPCBYTE lpGameAssemblyBase
) {
    if (NULL == lpGameAssemblyBase) {
        return FALSE;
    }

    BYTE abPatchBytes[3] = {
        0x41, 0xB1, 0x01        // MOV R9B, 1       ; bypassLimit=true
    };

    LPBYTE lpTargetPatchAddress = (LPBYTE) lpGameAssemblyBase + 0x0167D9D4;

    LogMessage("Patching item give rarity bug @ %p\n", lpTargetPatchAddress);
    if (!WriteExecMemory(
        lpTargetPatchAddress,
        abPatchBytes,
        sizeof(abPatchBytes)
    )) {
        LogMessage("Failed to patch item give rarity bug!\n");
        return FALSE;
    }

    LogMessage("Item give rarity bug patch applied successfully.\n");
    return TRUE;
}

DWORD WINAPI ThreadProc(
    _In_ LPVOID lpParam
) {
    if (!InitializeLogFile()) {
        MessageBoxA(
            NULL,
            "Failed to initialize log file!",
            "[Saleblazers FreeMove] Error",
            MB_OK | MB_ICONERROR
        );
    } else {
        LogMessage("Saleblazers QCUnlocker initializing..\n");
    }

    PATCH_CONFIG patchConfig = { 0 };
    LoadPatchConfig(&patchConfig);

    if ('\0' != patchConfig.szLoadedConfigPath[0]) {
        LogMessage("Loaded patch config from: '%s'\n", patchConfig.szLoadedConfigPath);
    } else {
        LogMessage("No patch config loaded, using defaults.\n");

        if (!CreateDefaultConfig("SaleblazersQCUnlocker.ini")) {
            LogMessage("Failed to create default INI config file\n");
        } else {
            LogMessage("Default INI config file created successfully\n");
        }
    }

    LogMessage(
        "Patch config:\n"
        " * BypassPlacementRestrictions = %s\n"
        " * UnlockQuantumConsoleDevMode = %s\n"
        " * ItemGiveRarityFix = %s\n",
        patchConfig.BypassPlacementRestrictions ? "enabled" : "disabled",
        patchConfig.UnlockQuantumConsoleDevMode ? "enabled" : "disabled",
        patchConfig.ItemGiveRarityFix ? "enabled" : "disabled"
    );

    LPCBYTE lpGameAssemblyBase = (LPCBYTE) GetModuleHandleA("GameAssembly.dll");
    if (NULL == lpGameAssemblyBase) {
        MessageBoxA(
            NULL,
            "Failed to get GameAssembly.dll base address",
            "[Saleblazers FreeMove] Error",
            MB_OK | MB_ICONERROR
        );
        return EXIT_FAILURE;
    }

    SIZE_T cbGameAssemblySize = GetModuleSize((HMODULE) lpGameAssemblyBase);
    if (0 == cbGameAssemblySize) {
        LogMessage("Failed to get GameAssembly.dll size\n");
        return EXIT_FAILURE;
    }
    
    LogMessage("GameAssembly.dll base @ %p\n", lpGameAssemblyBase);
    LogMessage("GameAssembly.dll size:  %llu bytes\n", cbGameAssemblySize);

    if (patchConfig.BypassPlacementRestrictions) {
        if (!PatchPlacementRestrictions(lpGameAssemblyBase)) {
            LogMessage("Failed to patch placement restrictions!\n");
        } else {
            LogMessage("Placement restriction bypass patch applied successfully.\n");
        }
    }

    if (patchConfig.UnlockQuantumConsoleDevMode) {
        if (!PatchQuantumConsoleDevMode(lpGameAssemblyBase)) {
            LogMessage("Failed to patch Quantum Console dev mode prologues!\n");
        } else {
            LogMessage("Quantum Console dev mode prologue patches applied successfully.\n");
        }

        if (!ApplyQCDevPatches(lpGameAssemblyBase, cbGameAssemblySize)) {
            LogMessage("Failed to apply QC dev patches!\n");
        } else {
            LogMessage("QC dev patches applied successfully.\n");
        }
    }

    if (patchConfig.ItemGiveRarityFix) {
        if (!PatchItemGiveRarityBug(lpGameAssemblyBase)) {
            LogMessage("Failed to patch item give rarity bug!\n");
        } else {
            LogMessage("Item give rarity bug patch applied successfully.\n");
        }
    }

    LogMessage("Patching complete, exiting thread..\n");

    return EXIT_SUCCESS;
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            g_hThread = CreateThread(
                NULL,
                0,
                ThreadProc,
                NULL,
                0,
                &g_dwThreadId
            );

            if (g_hThread == NULL) {
                MessageBoxA(
                    NULL,
                    "Failed to create 1337 hax thread",
                    "[SaleblazersQCUnlocker] Error",
                    MB_OK | MB_ICONERROR
                );
                return FALSE;
            }

            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            // Code to run when the DLL is unloaded
            break;
    }
    return TRUE;
}