#ifndef FORTUTL_H
#define FORTUTL_H

#include "fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS fort_driver_path(PDRIVER_OBJECT driver, PUNICODE_STRING regPath, PWSTR *outPath);

FORT_API NTSTATUS fort_file_read(HANDLE fileHandle, ULONG poolTag, PUCHAR *outData, DWORD *outSize);
FORT_API NTSTATUS fort_file_open(PCWSTR filePath, HANDLE *outHandle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTUTL_H
