/* Fort Firewall Driver Image Handling: Memory Module Loader */

#include "fortmm.h"

#include "fortmm_imp.h"

#if !defined(FORT_DRIVER)
#    define PAGE_SIZE 0x1000
#endif

/* MIN/MAX of address aligned */
#define MIN_ALIGNED(address, alignment) (LPVOID)((uintptr_t) (address) & ~((alignment) -1))
#define MAX_ALIGNED(value, alignment)   (((value) + (alignment) -1) & ~((alignment) -1))

typedef NTSTATUS(WINAPI *DriverImportsSetupProc)(PFORT_MODULE_IMP moduleImp);

typedef NTSTATUS(WINAPI *DriverCallbacksSetupProc)(PFORT_PROXYCB_INFO cbInfo);

typedef NTSTATUS(WINAPI *DriverEntryProc)(PDRIVER_OBJECT driver, PUNICODE_STRING regPath);

typedef struct
{
    PLOADEDMODULE pModule;
    PIMAGE_NT_HEADERS pNtHeaders;
    const PUCHAR lpData;
    DWORD dwSize;
    DWORD imageSize;
} FORT_MODULE_IMAGE;

typedef struct
{
    PUCHAR codeBase;
    PIMAGE_IMPORT_DESCRIPTOR importDesc;
    LPCSTR libName;
    PLOADEDMODULE libModule;
} FORT_MODULE_IMAGE_LIB;

static VOID ZeroDataSectionTable(
        PUCHAR pImage, const PIMAGE_NT_HEADERS pNtHeaders, PIMAGE_SECTION_HEADER section)
{
    /* Section doesn't contain data in the dll itself, but may define uninitialized data. */
    const DWORD sectionSize = pNtHeaders->OptionalHeader.SectionAlignment;
    if (sectionSize == 0)
        return; /* Ignore the empty section. */

    /* Always use position from file to support alignments smaller than page size. */
    const PUCHAR dest = pImage + section->VirtualAddress;
    RtlZeroMemory(dest, sectionSize);

    section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;

#ifdef FORT_DEBUG
    LOG("Loader Module: Zero Section: offset=%d size=%d\n", section->VirtualAddress, sectionSize);
#endif
}

static NTSTATUS CopySectionTable(PUCHAR pImage, const FORT_MODULE_IMAGE mi)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(mi.pNtHeaders);

    const int numberOfSections = mi.pNtHeaders->FileHeader.NumberOfSections;

    for (int i = 0; i < numberOfSections; ++i, ++section) {
        const DWORD sectionSize = section->SizeOfRawData;
        if (sectionSize == 0) {
            ZeroDataSectionTable(pImage, mi.pNtHeaders, section);
            continue;
        }

        if (section->VirtualAddress + sectionSize > mi.imageSize
                || section->PointerToRawData + sectionSize > mi.dwSize)
            return STATUS_INVALID_IMAGE_FORMAT;

        /* Always use position from file to support alignments smaller than page size. */
        PUCHAR dest = pImage + section->VirtualAddress;
        RtlCopyMemory(dest, mi.lpData + section->PointerToRawData, sectionSize);

        /* NOTE: On 64bit systems we truncate to 32bit here but expand
         * again later when "PhysicalAddress" is used.
         */
        section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;

#ifdef FORT_DEBUG
        LOG("Loader Module: Copy Section: src-offset=%x offset=%x size=%x data=%x\n",
                section->PointerToRawData, section->VirtualAddress, sectionSize, *(PDWORD) dest);
#endif
    }

    return STATUS_SUCCESS;
}

static void PatchAddressRelocations(
        PUCHAR codeBase, PIMAGE_BASE_RELOCATION relocation, ptrdiff_t locationDelta)
{
    const PUCHAR dest = codeBase + relocation->VirtualAddress;
    PUSHORT relInfo = (PUSHORT) ((PUCHAR) relocation + sizeof(IMAGE_BASE_RELOCATION));
    const DWORD relInfoCount = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2;

#ifdef FORT_DEBUG
    LOG("Loader Module: Relocation: dest=%p count=%d\n", dest, relInfoCount);
#endif

    for (DWORD i = 0; i < relInfoCount; ++i, ++relInfo) {
        const INT type = *relInfo >> 12; /* the upper 4 bits define the type of relocation */
        const INT offset = *relInfo & 0xfff; /* the lower 12 bits define the offset */

        switch (type) {
        case IMAGE_REL_BASED_ABSOLUTE:
            break; /* skip relocation */

        case IMAGE_REL_BASED_HIGHLOW: {
            /* change complete 32 bit address */
            PUCHAR *patchAddrHL = (PUCHAR *) (dest + offset);
            *patchAddrHL += locationDelta;
        } break;

#if defined(_WIN64)
        case IMAGE_REL_BASED_DIR64: {
            PUCHAR *patchAddr64 = (PUCHAR *) (dest + offset);
            *patchAddr64 += locationDelta;
        } break;
#endif

        default:
            break;
        }
    }
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

#ifdef FORT_DEBUG
    LOG("Loader Module: Relocation: size=%d delta=%d\n", directory->Size, locationDelta);
#endif

    if (directory->Size == 0) {
        return (locationDelta == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    PIMAGE_BASE_RELOCATION relocation =
            (PIMAGE_BASE_RELOCATION) (codeBase + directory->VirtualAddress);

    while (relocation->VirtualAddress > 0) {
        PatchAddressRelocations(codeBase, relocation, locationDelta);

        /* Advance to next relocation block */
        relocation = (PIMAGE_BASE_RELOCATION) ((PCHAR) relocation + relocation->SizeOfBlock);
    }

    return STATUS_SUCCESS;
}

/* Build the import address table: Library functions. */
static NTSTATUS BuildImportTableLibrary(PFORT_MODULE_IMP moduleImp, const FORT_MODULE_IMAGE_LIB mil)
{
    NTSTATUS status = STATUS_SUCCESS;

    ModuleGetProcAddressFallbackProc moduleGetProcAddressFallback =
            moduleImp->moduleGetProcAddressFallback;

    const DWORD originalFirstThunk = (mil.importDesc->OriginalFirstThunk != 0)
            ? mil.importDesc->OriginalFirstThunk
            : mil.importDesc->FirstThunk;
    uintptr_t *thunkRef = (uintptr_t *) (mil.codeBase + originalFirstThunk);
    FARPROC *funcRef = (FARPROC *) (mil.codeBase + mil.importDesc->FirstThunk);

    for (; *thunkRef; ++thunkRef, ++funcRef) {
        LPCSTR funcName;
        if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
            funcName = (LPCSTR) IMAGE_ORDINAL(*thunkRef);
        } else {
            const PIMAGE_IMPORT_BY_NAME thunkData =
                    (PIMAGE_IMPORT_BY_NAME) (mil.codeBase + (*thunkRef));
            funcName = (LPCSTR) &thunkData->Name;
        }

        *funcRef = moduleGetProcAddressFallback(moduleImp, mil.libModule, funcName);
        if (*funcRef == NULL) {
            LOG("Loader Module: Error: Procedure Not Found: %s: %s\n", mil.libName, funcName);
            status = STATUS_PROCEDURE_NOT_FOUND;
        } else {
#ifdef FORT_DEBUG
            LOG("Loader Module: Import: %s: %s: %p\n", mil.libName, funcName, *funcRef);
#endif
        }
    }

    return status;
}

static NTSTATUS BuildImportTableEntries(
        PFORT_MODULE_IMP moduleImp, PUCHAR codeBase, PIMAGE_DATA_DIRECTORY directory)
{
    NTSTATUS status;

    GetModuleInfoFallbackProc getModuleInfoFallback = moduleImp->getModuleInfoFallback;
    BuildImportTableLibraryBeginProc buildImportTableLibraryBegin =
            moduleImp->buildImportTableLibraryBegin;

    PIMAGE_IMPORT_DESCRIPTOR importDesc =
            (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);

    FORT_MODULE_IMAGE_LIB moduleLib = {
        .codeBase = codeBase,
    };

    for (; importDesc != NULL && importDesc->Name != 0; ++importDesc) {
        LPCSTR libName = (LPCSTR) (codeBase + importDesc->Name);

        LOADEDMODULE libModule;
        status = getModuleInfoFallback(moduleImp, &libModule, libName);
        if (!NT_SUCCESS(status)) {
            LOG("Loader Module: Error: Module Not Found: %s\n", libName);
            break;
        }

        buildImportTableLibraryBegin(moduleImp, libName);

        moduleLib.importDesc = importDesc;
        moduleLib.libName = libName;
        moduleLib.libModule = &libModule;

        status = BuildImportTableLibrary(moduleImp, moduleLib);
        if (!NT_SUCCESS(status)) {
            LOG("Loader Module: Library Import Error: %s\n", libName);
            break;
        }
    }

    return status;
}

/* Build the import address table. */
static NTSTATUS BuildImportTable(const FORT_MODULE_IMAGE mi)
{
    NTSTATUS status;

    PIMAGE_DATA_DIRECTORY directory =
            &(mi.pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    if (directory->Size == 0)
        return STATUS_SUCCESS;

    DriverImportsSetupProc driverImportsSetup =
            (DriverImportsSetupProc) ModuleGetProcAddress(mi.pModule, "DriverImportsSetup");
    if (driverImportsSetup == NULL)
        return STATUS_DRIVER_ORDINAL_NOT_FOUND;

    PAUX_MODULE_EXTENDED_INFO modules;
    DWORD modulesCount;
    status = GetModuleInfoList(&modules, &modulesCount);
    if (!NT_SUCCESS(status))
        return status;

    FORT_MODULE_IMP moduleImp;
    InitModuleImporter(&moduleImp, modules, modulesCount);

    driverImportsSetup(&moduleImp);

    status = moduleImp.buildImportTableEntriesBegin(&moduleImp, mi.pModule, mi.pNtHeaders);
    if (!NT_SUCCESS(status))
        return status;

    if (status == STATUS_ALREADY_COMPLETE) {
        status = STATUS_SUCCESS;
    } else {
        status = BuildImportTableEntries(&moduleImp, mi.pModule->codeBase, directory);
    }

    moduleImp.buildImportTableEntriesEnd(&moduleImp, status);

    /* Free the modules allocated data */
    FreeModuleInfoList(modules);

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

    /* Check entry point */
    if (pNtHeaders->OptionalHeader.AddressOfEntryPoint == 0)
        return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

    return TRUE;
}

inline static BOOL CheckPEHeaderOptionalValid(const PIMAGE_NT_HEADERS pNtHeaders, DWORD dwSize)
{
    /* Check NT header for valid signature */
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    /* Check size of optional headerss */
    if (dwSize < pNtHeaders->OptionalHeader.SizeOfHeaders)
        return FALSE;

    /* Check for the correct architecture */
    if (pNtHeaders->FileHeader.Machine !=
#if defined(_M_ARM64)
            IMAGE_FILE_MACHINE_ARM64
#elif defined(_WIN64)
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

static BOOL IsPEHeaderValid(const FORT_MODULE_IMAGE mi)
{
    const PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) mi.lpData;

    /* Check DOS header for valid signature */
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    /* Make sure size is at least size of headers */
    if (mi.dwSize < (sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER)))
        return FALSE;

    if (mi.dwSize < (pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)))
        return FALSE;

    /* Check for optional headers */
    const PIMAGE_NT_HEADERS pNtHeaders =
            (PIMAGE_NT_HEADERS) & ((PUCHAR) mi.lpData)[pDosHeader->e_lfanew];

    return CheckPEHeaderOptionalValid(pNtHeaders, mi.dwSize);
}

static NTSTATUS InitializeModuleImage(const FORT_MODULE_IMAGE mi)
{
    NTSTATUS status;

#ifdef FORT_DEBUG
    LOG("Loader Module: Init Image: SizeOfHeaders=%d EntryPoint=%d ImageBase=%x\n",
            mi.pNtHeaders->OptionalHeader.SizeOfHeaders,
            mi.pNtHeaders->OptionalHeader.AddressOfEntryPoint,
            mi.pNtHeaders->OptionalHeader.ImageBase);
#endif

    PUCHAR pImage = mi.pModule->codeBase;

    /* Copy PE header */
    RtlCopyMemory(pImage, mi.lpData, mi.pNtHeaders->OptionalHeader.SizeOfHeaders);

    /* Update position of the image base */
    PIMAGE_NT_HEADERS pNtHeaders = GetModuleNtHeaders(pImage);
    pNtHeaders->OptionalHeader.ImageBase = (uintptr_t) pImage;

    /* Copy section table */
    FORT_MODULE_IMAGE newModule = mi;
    newModule.pNtHeaders = pNtHeaders;

    status = CopySectionTable(pImage, newModule);
    if (!NT_SUCCESS(status))
        return status;

    /* Adjust base address of imported data */
    const ptrdiff_t locationDelta = pImage - (PUCHAR) mi.pNtHeaders->OptionalHeader.ImageBase;

    if (locationDelta != 0) {
        PerformBaseRelocation(pImage, pNtHeaders, locationDelta);
    }

    /* Adjust function table of imports */
    status = BuildImportTable(newModule);
    if (!NT_SUCCESS(status))
        return status;

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS LoadModuleFromMemory(PLOADEDMODULE pModule, const PUCHAR lpData, DWORD dwSize)
{
    NTSTATUS status;

    FORT_MODULE_IMAGE module = {
        .pModule = pModule,
        .lpData = lpData,
        .dwSize = dwSize,
    };

    /* Check header */
    if (!IsPEHeaderValid(module))
        return STATUS_INVALID_IMAGE_FORMAT;

    const PIMAGE_NT_HEADERS pNtHeaders = GetModuleNtHeaders(lpData);
    const DWORD imageSize = MAX_ALIGNED(pNtHeaders->OptionalHeader.SizeOfImage, PAGE_SIZE);

#ifdef FORT_DEBUG
    LOG("Loader Module: data=%p size=%d imageSize=%d\n", lpData, dwSize, imageSize);
#endif

    /* Allocate the region */
    PUCHAR pImage = fort_mem_alloc_exec(imageSize, FORT_LOADER_POOL_TAG);
    if (pImage == NULL)
        return STATUS_NO_MEMORY;

#ifdef FORT_DEBUG
    LOG("Loader Module: image=%p\n", pImage);
#endif

    pModule->codeBase = pImage;

    module.pNtHeaders = pNtHeaders;
    module.imageSize = imageSize;

    status = InitializeModuleImage(module);

    if (!NT_SUCCESS(status)) {
        pModule->codeBase = NULL;
        fort_mem_free(pImage, FORT_LOADER_POOL_TAG);
        return status;
    }

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

NTSTATUS SetupModuleCallbacks(PLOADEDMODULE pModule, PFORT_PROXYCB_INFO cbInfo)
{
    DriverCallbacksSetupProc cbSetup =
            (DriverCallbacksSetupProc) ModuleGetProcAddress(pModule, "DriverCallbacksSetup");
    if (cbSetup == NULL)
        return STATUS_PROCEDURE_NOT_FOUND;

#ifdef FORT_DEBUG
    LOG("Loader Module: Setup Callbacks: %p data=%x\n", cbSetup, *(PDWORD) (PVOID) &cbSetup);
#endif

    return cbSetup(cbInfo);
}

FORT_API NTSTATUS CallModuleEntry(
        PLOADEDMODULE pModule, PDRIVER_OBJECT driver, PUNICODE_STRING regPath)
{
    DriverEntryProc driverEntry = (DriverEntryProc) ModuleGetProcAddress(pModule, "DriverEntry");
    if (driverEntry == NULL)
        return STATUS_PROCEDURE_NOT_FOUND;

#ifdef FORT_DEBUG
    LOG("Loader Module: Driver Entry: %p data=%x\n", driverEntry, *(PDWORD) (PVOID) &driverEntry);
#endif

    return driverEntry(driver, regPath);
}
