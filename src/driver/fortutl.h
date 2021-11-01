#ifndef FORTUTL_H
#define FORTUTL_H

#include "fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS fort_reg_value(HANDLE regKey, PUNICODE_STRING valueName, PWSTR *outData);

FORT_API NTSTATUS fort_driver_path(PDRIVER_OBJECT driver, PUNICODE_STRING regPath, PWSTR *outPath);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTUTL_H
