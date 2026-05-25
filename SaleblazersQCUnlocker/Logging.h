#ifndef _SALEBLAZERS_QC_UNLOCKER_LOGGING_H_
#define _SALEBLAZERS_QC_UNLOCKER_LOGGING_H_

#include "QCUnlocker.h"

#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>

BOOL InitializeLogFile(VOID);

VOID LogMessage(
    _In_ LPCSTR lpFormat, 
    ...
);

#endif // _SALEBLAZERS_QC_UNLOCKER_LOGGING_H_