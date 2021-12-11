/* Fort Firewall Driver Image Handling: Memory Module Loader */

#include "fortmm.h"

#if defined(FORT_DRIVER)
#    include <aux_klib.h>
#    include <ntimage.h>

#    define IS_INTRESOURCE(_r) ((((ULONG_PTR) (_r)) >> 16) == 0)
#else
#    define PAGE_SIZE 0x1000
#endif

#define ARCHITECTURE_TYPE_X86 0x00000000
#define ARCHITECTURE_TYPE_X64 0x00000001

/* MIN/MAX of address aligned */
#define MIN_ALIGNED(address, alignment) (LPVOID)((uintptr_t) (address) & ~((alignment) -1))
#define MAX_ALIGNED(value, alignment)   (((value) + (alignment) -1) & ~((alignment) -1))

#define fort_nt_headers(pImage)                                                                    \
    ((PIMAGE_NT_HEADERS) & ((PUCHAR) (pImage))[((PIMAGE_DOS_HEADER) pImage)->e_lfanew])

typedef NTSTATUS(WINAPI *DriverCallbackEntryProc)(
        PDRIVER_OBJECT driver, PUNICODE_STRING regPath, PFORT_PROXYCB_INFO cbInfo);

static NTSTATUS GetModuleInfo(PLOADEDMODULE pModule, LPCSTR name,
        const PAUX_MODULE_EXTENDED_INFO modules, DWORD modulesCount)
{
    PAUX_MODULE_EXTENDED_INFO module = modules;
    for (DWORD i = 0; i < modulesCount; ++i, ++module) {
        if (_stricmp(name, &module->FullPathName[module->FileNameOffset]) == 0) {
            pModule->codeBase = module->BasicInfo.ImageBase;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_DRIVER_ORDINAL_NOT_FOUND;
}

static NTSTATUS GetModuleInfoList(PAUX_MODULE_EXTENDED_INFO *outModules, DWORD *outModulesCount)
{
    NTSTATUS status;

    status = AuxKlibInitialize();
    if (!NT_SUCCESS(status))
        return status;

    ULONG size = 0;
    status = AuxKlibQueryModuleInformation(&size, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);
    if (!NT_SUCCESS(status) || size == 0)
        return NT_SUCCESS(status) ? STATUS_INVALID_BUFFER_SIZE : status;

    PUCHAR data = fort_mem_alloc(size, FORT_LOADER_POOL_TAG);
    if (data == NULL)
        return STATUS_NO_MEMORY;

    status = AuxKlibQueryModuleInformation(&size, sizeof(AUX_MODULE_EXTENDED_INFO), data);
    if (!NT_SUCCESS(status)) {
        fort_mem_free(data, FORT_LOADER_POOL_TAG);
        return status;
    }

    *outModules = (PAUX_MODULE_EXTENDED_INFO) data;
    *outModulesCount = size / sizeof(AUX_MODULE_EXTENDED_INFO);

    return status;
}

static NTSTATUS CopySectionTable(
        PUCHAR codeBase, const PIMAGE_NT_HEADERS pNtHeaders, const PUCHAR pData, SIZE_T size)
{
    PIMAGE_NT_HEADERS pHeaders = fort_nt_headers(codeBase);
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pHeaders);

    const int numberOfSections = pNtHeaders->FileHeader.NumberOfSections;

    for (int i = 0; i < numberOfSections; ++i, ++section) {
        if (section->SizeOfRawData == 0) {
            /* Section doesn't contain data in the dll itself, but may define uninitialized data. */
            const DWORD sectionSize = pNtHeaders->OptionalHeader.SectionAlignment;
            if (sectionSize > 0) {
                /* Always use position from file to support alignments smaller than page size. */
                const PUCHAR dest = codeBase + section->VirtualAddress;
                RtlZeroMemory(dest, sectionSize);

                section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;

                DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                        "FORT: Loader Module: Zero Section: offset=%d size=%d\n",
                        section->VirtualAddress, sectionSize);
            }
        } else {
            const DWORD sectionSize = section->SizeOfRawData;
            if (size < (SIZE_T) section->PointerToRawData + sectionSize)
                return STATUS_INVALID_IMAGE_FORMAT;

            /* Always use position from file to support alignments smaller than page size. */
            const PUCHAR dest = codeBase + section->VirtualAddress;
            RtlCopyMemory(dest, pData + section->PointerToRawData, sectionSize);

            /* NOTE: On 64bit systems we truncate to 32bit here but expand
             * again later when "PhysicalAddress" is used.
             */
            section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;

            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: Loader Module: Copy Section: src-offset=%d offset=%d size=%d data=%x\n",
                    section->PointerToRawData, section->VirtualAddress, sectionSize,
                    *(PDWORD) dest);
        }
    }

    return STATUS_SUCCESS;
}

/*
 * The DLL's preferred load address conflicts with memory that's already in use
 * so we need to 'rebase' the DLL by loading it at a different address that does
 * not overlap and then adjust all addresses.
 */
static NTSTATUS PerformBaseRelocation(
        PUCHAR codeBase, PIMAGE_NT_HEADERS pHeaders, ptrdiff_t locationDelta)
{
    PIMAGE_DATA_DIRECTORY directory =
            &(pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
    if (directory->Size == 0) {
        return (locationDelta == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "FORT: Loader Module: Base Relocation: %d\n", locationDelta);

    PIMAGE_BASE_RELOCATION relocation =
            (PIMAGE_BASE_RELOCATION) (codeBase + directory->VirtualAddress);

    while (relocation->VirtualAddress > 0) {
        const PUCHAR dest = codeBase + relocation->VirtualAddress;
        PUSHORT relInfo = (PUSHORT) ((PUCHAR) relocation + sizeof(IMAGE_BASE_RELOCATION));
        const DWORD relInfoCount = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2;

        for (DWORD i = 0; i < relInfoCount; ++i, ++relInfo) {
            const INT type = *relInfo >> 12; /* the upper 4 bits define the type of relocation */
            const INT offset = *relInfo & 0xfff; /* the lower 12 bits define the offset */

            switch (type) {
            case IMAGE_REL_BASED_ABSOLUTE:
                break; /* skip relocation */

            case IMAGE_REL_BASED_HIGHLOW:
                /* change complete 32 bit address */
                DWORD *patchAddrHL = (PDWORD) (dest + offset);
                *patchAddrHL += (DWORD) locationDelta;
                break;

#ifdef _WIN64
            case IMAGE_REL_BASED_DIR64:
                ULONGLONG *patchAddr64 = (PULONGLONG) (dest + offset);
                *patchAddr64 += (ULONGLONG) locationDelta;
                break;
#endif

            default:
                break;
            }
        }

        /* Advance to next relocation block */
        relocation = (PIMAGE_BASE_RELOCATION) ((PCHAR) relocation + relocation->SizeOfBlock);
    }

    return STATUS_SUCCESS;
}

/* Build the import address table: Library functions. */
static NTSTATUS BuildImportTableLibrary(PUCHAR codeBase, const PIMAGE_IMPORT_DESCRIPTOR importDesc,
        LPCSTR libName, LOADEDMODULE libModule)
{
    NTSTATUS status = STATUS_SUCCESS;

    const DWORD originalFirstThunk = (importDesc->OriginalFirstThunk != 0)
            ? importDesc->OriginalFirstThunk
            : importDesc->FirstThunk;
    uintptr_t *thunkRef = (uintptr_t *) (codeBase + originalFirstThunk);
    FARPROC *funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);

    for (; *thunkRef; ++thunkRef, ++funcRef) {
        LPCSTR funcName;
        if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
            funcName = (LPCSTR) IMAGE_ORDINAL(*thunkRef);
        } else {
            const PIMAGE_IMPORT_BY_NAME thunkData =
                    (PIMAGE_IMPORT_BY_NAME) (codeBase + (*thunkRef));
            funcName = (LPCSTR) &thunkData->Name;
        }

        *funcRef = ModuleGetProcAddress(&libModule, funcName);
        if (*funcRef == 0) {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: Loader Module: Error: Procedure Not Found: %s: %s\n", libName, funcName);
            status = STATUS_PROCEDURE_NOT_FOUND;
        } else {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: Loader Module: Import: %s: %s: %p\n", libName, funcName, *funcRef);
        }
    }

    return status;
}

/* Build the import address table. */
static NTSTATUS BuildImportTable(PUCHAR codeBase, PIMAGE_NT_HEADERS pHeaders)
{
    NTSTATUS status;

    PIMAGE_DATA_DIRECTORY directory =
            &(pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    if (directory->Size == 0)
        return STATUS_SUCCESS;

    PAUX_MODULE_EXTENDED_INFO modules;
    DWORD modulesCount;
    status = GetModuleInfoList(&modules, &modulesCount);
    if (!NT_SUCCESS(status))
        return status;

    status = STATUS_SUCCESS;

    PIMAGE_IMPORT_DESCRIPTOR importDesc =
            (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);

    for (; importDesc != NULL && importDesc->Name != 0; ++importDesc) {
        LPCSTR libName = (LPCSTR) (codeBase + importDesc->Name);

        LOADEDMODULE libModule;
        if (!NT_SUCCESS(GetModuleInfo(&libModule, libName, modules, modulesCount))) {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: Loader Module: Error: Module Not Found: %s\n", libName);
            status = STATUS_PROCEDURE_NOT_FOUND;
            break;
        }

        status = BuildImportTableLibrary(codeBase, importDesc, libName, libModule);
        if (!NT_SUCCESS(status)) {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: Loader Module: Library Import Error: %s\n", libName);
            break;
        }
    }

    /* Free the modules allocated data */
    fort_mem_free(modules, FORT_LOADER_POOL_TAG);

    return status;
}

static BOOL CheckPEHeaderSections(const PIMAGE_NT_HEADERS pNtHeaders)
{
    SIZE_T lastSectionEnd = 0;
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);

    const int numberOfSections = pNtHeaders->FileHeader.NumberOfSections;

    for (int i = 0; i < numberOfSections; ++i, ++pSection) {
        SIZE_T endOfSection;
        if (pSection->SizeOfRawData == 0) {
            /* Section without data in the DLL */
            endOfSection =
                    (SIZE_T) pSection->VirtualAddress + pNtHeaders->OptionalHeader.SectionAlignment;
        } else {
            endOfSection = (SIZE_T) pSection->VirtualAddress + pSection->SizeOfRawData;
        }

        if (endOfSection > lastSectionEnd) {
            lastSectionEnd = endOfSection;
        }
    }

    const SIZE_T imageSize = MAX_ALIGNED(pNtHeaders->OptionalHeader.SizeOfImage, PAGE_SIZE);
    if (imageSize != MAX_ALIGNED(lastSectionEnd, PAGE_SIZE))
        return STATUS_INVALID_IMAGE_FORMAT;

    return TRUE;
}

static BOOL IsPEHeaderValid(PVOID lpData, DWORD dwSize)
{
    const PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) lpData;

    /* Check DOS header for valid signature */
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    /* Make sure size is at least size of PE header */
    if (dwSize < (sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER)))
        return FALSE;

    /* Check for optional headers */
    const PIMAGE_NT_HEADERS pNtHeaders =
            (PIMAGE_NT_HEADERS) & ((PUCHAR) lpData)[pDosHeader->e_lfanew];

    /* Check NT header for valid signature */
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    /* Check sizes */
    if (dwSize < sizeof(IMAGE_DOS_HEADER) || dwSize < pNtHeaders->OptionalHeader.SizeOfHeaders
            || dwSize < (pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)))
        return FALSE;

    /* Check for the correct architecture */
    if (pNtHeaders->FileHeader.Machine !=
#ifdef _WIN64
            IMAGE_FILE_MACHINE_AMD64
#else
            IMAGE_FILE_MACHINE_I386
#endif
    )
        return FALSE;

    /* Check to see if the image is really an executable file */
    if ((pNtHeaders->FileHeader.Characteristics & (IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL))
            == 0)
        return FALSE;

    /* Check sections */
    return CheckPEHeaderSections(pNtHeaders);
}

static NTSTATUS InitializeModuleImage(
        PUCHAR pImage, const PIMAGE_NT_HEADERS pNtHeaders, const PUCHAR lpData, DWORD dwSize)
{
    NTSTATUS status;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "FORT: Loader Module: Init Image: Headers size=%d Entry point=%d\n",
            pNtHeaders->OptionalHeader.SizeOfHeaders,
            pNtHeaders->OptionalHeader.AddressOfEntryPoint);

    /* Copy PE header to code */
    RtlCopyMemory(pImage, lpData, pNtHeaders->OptionalHeader.SizeOfHeaders);

    /* Update position of the image base */
    PIMAGE_NT_HEADERS pHeaders = fort_nt_headers(pImage);
    pHeaders->OptionalHeader.ImageBase = (uintptr_t) pImage;

    /* Copy section table */
    status = CopySectionTable(pImage, pNtHeaders, lpData, dwSize);
    if (!NT_SUCCESS(status))
        return status;

    /* Adjust base address of imported data */
    const ptrdiff_t locationDelta =
            (ptrdiff_t) pHeaders->OptionalHeader.ImageBase - pNtHeaders->OptionalHeader.ImageBase;
    if (locationDelta != 0) {
        PerformBaseRelocation(pImage, pHeaders, locationDelta);
    }

    /* Adjust function table of imports */
    status = BuildImportTable(pImage, pHeaders);
    if (!NT_SUCCESS(status))
        return status;

    /* Check entry point */
    if (pHeaders->OptionalHeader.AddressOfEntryPoint == 0)
        return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS LoadModuleFromMemory(PLOADEDMODULE pModule, PUCHAR lpData, DWORD dwSize)
{
    NTSTATUS status;

    /* Check header */
    if (!IsPEHeaderValid(lpData, dwSize))
        return STATUS_INVALID_IMAGE_FORMAT;

    PIMAGE_NT_HEADERS pNtHeaders = fort_nt_headers(lpData);
    const DWORD imageSize = MAX_ALIGNED(pNtHeaders->OptionalHeader.SizeOfImage, PAGE_SIZE);

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "FORT: Loader Module: Data size=%d Image size=%d Aligned Image size=%d\n", dwSize,
            pNtHeaders->OptionalHeader.SizeOfImage, imageSize);

    /* Allocate the region */
    PUCHAR pImage = fort_mem_exec_alloc(imageSize, FORT_LOADER_POOL_TAG);
    if (pImage == NULL)
        return STATUS_NO_MEMORY;

    status = InitializeModuleImage(pImage, pNtHeaders, lpData, dwSize);

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Module: Image Base: %p %x\n",
            pImage, status);

    if (!NT_SUCCESS(status)) {
        fort_mem_free(pImage, FORT_LOADER_POOL_TAG);
        return status;
    }

    pModule->codeBase = pImage;

    return STATUS_SUCCESS;
}

/* Free all resources allocated for a loaded module */
FORT_API void UnloadModule(PLOADEDMODULE pModule)
{
    if (pModule->codeBase != NULL) {
        /* Free the module's allocated data */
        fort_mem_free(pModule->codeBase, FORT_LOADER_POOL_TAG);
        pModule->codeBase = NULL;
    }
}

FORT_API NTSTATUS CallModuleEntry(PLOADEDMODULE pModule, PDRIVER_OBJECT driver,
        PUNICODE_STRING regPath, PFORT_PROXYCB_INFO cbInfo)
{
    DriverCallbackEntryProc driverEntry =
            (DriverCallbackEntryProc) ModuleGetProcAddress(pModule, "DriverCallbackEntry");
    if (driverEntry == NULL)
        return STATUS_PROCEDURE_NOT_FOUND;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "FORT: Loader Module: Entry Proc: %p data=%x\n", driverEntry,
            *(PDWORD) (PVOID) &driverEntry);

    return driverEntry(driver, regPath, cbInfo);
}

/* Retrieve address of an exported function from the loaded module. */
FORT_API FARPROC ModuleGetProcAddress(PLOADEDMODULE pModule, LPCSTR funcName)
{
    const PUCHAR codeBase = pModule->codeBase;
    const PIMAGE_NT_HEADERS pHeaders = fort_nt_headers(codeBase);

    const PIMAGE_DATA_DIRECTORY directory =
            &(pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    if (directory->Size == 0)
        return NULL; /* no export table found */

    const PIMAGE_EXPORT_DIRECTORY exports =
            (PIMAGE_EXPORT_DIRECTORY) (codeBase + directory->VirtualAddress);
    if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0)
        return NULL; /* Our modules must export 3 functions. */

    int idx = -1;

    if (HIWORD(funcName) == 0) {
        /* Load function by ordinal value */
        idx = LOWORD(funcName) - exports->Base;
    } else {
        /* Search function name in list of exported names */
        DWORD *nameRef = (DWORD *) (codeBase + exports->AddressOfNames);
        WORD *ordinal = (WORD *) (codeBase + exports->AddressOfNameOrdinals);

        for (DWORD i = 0; i < exports->NumberOfNames; ++i, ++nameRef, ++ordinal) {
            if (strcmp(funcName, (const char *) (codeBase + *nameRef)) == 0) {
                idx = *ordinal;
                break;
            }
        }
    }

    if (idx < 0 || idx > (int) exports->NumberOfFunctions)
        return NULL; /* exported symbol not found or name <-> ordinal number don't match */

    /* AddressOfFunctions contains the RVAs to the "real" functions */
    return (FARPROC) (PVOID) (codeBase
            + *(DWORD *) (codeBase + exports->AddressOfFunctions + (SIZE_T) idx * 4));
}
