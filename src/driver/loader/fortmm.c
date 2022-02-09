/* Fort Firewall Driver Image Handling: Memory Module Loader */

#include "fortmm.h"

#if !defined(FORT_DRIVER)
#    define PAGE_SIZE 0x1000
#endif

#define ARCHITECTURE_TYPE_X86 0x00000000
#define ARCHITECTURE_TYPE_X64 0x00000001

/* MIN/MAX of address aligned */
#define MIN_ALIGNED(address, alignment) (LPVOID)((uintptr_t) (address) & ~((alignment) -1))
#define MAX_ALIGNED(value, alignment)   (((value) + (alignment) -1) & ~((alignment) -1))

typedef NTSTATUS(WINAPI *DriverCallbacksSetupProc)(PFORT_PROXYCB_INFO cbInfo);

typedef NTSTATUS(WINAPI *DriverEntryProc)(PDRIVER_OBJECT driver, PUNICODE_STRING regPath);

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

static NTSTATUS CopySectionTable(PUCHAR pImage, PIMAGE_NT_HEADERS pNtHeaders, const PUCHAR lpData,
        DWORD dwSize, DWORD imageSize)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNtHeaders);

    const int numberOfSections = pNtHeaders->FileHeader.NumberOfSections;

    for (int i = 0; i < numberOfSections; ++i, ++section) {
        const DWORD sectionSize = section->SizeOfRawData;
        if (sectionSize == 0) {
            ZeroDataSectionTable(pImage, pNtHeaders, section);
            continue;
        }

        if (section->VirtualAddress + sectionSize > imageSize
                || section->PointerToRawData + sectionSize > dwSize)
            return STATUS_INVALID_IMAGE_FORMAT;

        /* Always use position from file to support alignments smaller than page size. */
        PUCHAR dest = pImage + section->VirtualAddress;
        RtlCopyMemory(dest, lpData + section->PointerToRawData, sectionSize);

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

#ifdef _WIN64
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
static NTSTATUS BuildImportTableLibrary(PUCHAR codeBase, const PIMAGE_IMPORT_DESCRIPTOR importDesc,
        LPCSTR libName, PLOADEDMODULE libModule, PLOADEDMODULE forwardModule)
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

        if (forwardModule != NULL) {
            *funcRef = ModuleGetProcAddress(forwardModule, funcName);
            if (*funcRef != NULL) {
#ifdef FORT_DEBUG
                LOG("Loader Module: Import forwarded: %s: %s: %p\n", libName, funcName, *funcRef);
#endif
                continue;
            }
        }

        *funcRef = ModuleGetProcAddress(libModule, funcName);
        if (*funcRef == NULL) {
            LOG("Loader Module: Error: Procedure Not Found: %s: %s\n", libName, funcName);
            status = STATUS_PROCEDURE_NOT_FOUND;
        } else {
#ifdef FORT_DEBUG
            LOG("Loader Module: Import: %s: %s: %p\n", libName, funcName, *funcRef);
#endif
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

#if defined(FORT_WIN7_COMPAT)
    LOADEDMODULE kernelModule = { NULL };
    {
        RTL_OSVERSIONINFOW osvi;
        RtlZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        RtlGetVersion(&osvi);
        const BOOL isWindows7 = (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1);

        if (!isWindows7) {
            GetModuleInfo(&kernelModule, "ntoskrnl.exe", modules, modulesCount);
        }
    }
#endif

    status = STATUS_SUCCESS;

    PIMAGE_IMPORT_DESCRIPTOR importDesc =
            (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);

    for (; importDesc != NULL && importDesc->Name != 0; ++importDesc) {
        LPCSTR libName = (LPCSTR) (codeBase + importDesc->Name);

        LOADEDMODULE libModule;
        if (!NT_SUCCESS(GetModuleInfo(&libModule, libName, modules, modulesCount))) {
            LOG("Loader Module: Error: Module Not Found: %s\n", libName);
            status = STATUS_PROCEDURE_NOT_FOUND;
            break;
        }

        PLOADEDMODULE forwardModule = NULL;
#if defined(FORT_WIN7_COMPAT)
        if (kernelModule.codeBase != NULL && _stricmp(libName, "hal.dll") == 0) {
            /* Functions of HAL.dll are exported from kernel on Windows 8+ */
            LOG("Loader Module: Forward to kernel: %s\n", libName);
            forwardModule = &kernelModule;
        }
#endif

        status = BuildImportTableLibrary(codeBase, importDesc, libName, &libModule, forwardModule);
        if (!NT_SUCCESS(status)) {
            LOG("Loader Module: Library Import Error: %s\n", libName);
            break;
        }
    }

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

    return TRUE;
}

static BOOL IsPEHeaderValid(PVOID lpData, DWORD dwSize)
{
    const PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) lpData;

    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE /* Check DOS header for valid signature */
            /* Make sure size is at least size of headers */
            || dwSize < (sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER))
            || dwSize < (pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)))
        return FALSE;

    /* Check for optional headers */
    const PIMAGE_NT_HEADERS pNtHeaders =
            (PIMAGE_NT_HEADERS) & ((PUCHAR) lpData)[pDosHeader->e_lfanew];

    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE /* Check NT header for valid signature */
            /* Check size of optional headerss */
            || dwSize < pNtHeaders->OptionalHeader.SizeOfHeaders
            /* Check for the correct architecture */
            || pNtHeaders->FileHeader.Machine !=
#ifdef _WIN64
                    IMAGE_FILE_MACHINE_AMD64
#else
                    IMAGE_FILE_MACHINE_I386
#endif
            /* Check to see if the image is really an executable file */
            || (pNtHeaders->FileHeader.Characteristics
                       & (IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL))
                    == 0)
        return FALSE;

    /* Check sections */
    return CheckPEHeaderSections(pNtHeaders);
}

static NTSTATUS InitializeModuleImage(PUCHAR pImage, const PIMAGE_NT_HEADERS lpNtHeaders,
        const PUCHAR lpData, DWORD dwSize, DWORD imageSize)
{
    NTSTATUS status;

#ifdef FORT_DEBUG
    LOG("Loader Module: Init Image: SizeOfHeaders=%d EntryPoint=%d ImageBase=%x\n",
            lpNtHeaders->OptionalHeader.SizeOfHeaders,
            lpNtHeaders->OptionalHeader.AddressOfEntryPoint, lpNtHeaders->OptionalHeader.ImageBase);
#endif

    /* Copy PE header */
    RtlCopyMemory(pImage, lpData, lpNtHeaders->OptionalHeader.SizeOfHeaders);

    /* Update position of the image base */
    PIMAGE_NT_HEADERS pNtHeaders = GetModuleNtHeaders(pImage);
    pNtHeaders->OptionalHeader.ImageBase = (uintptr_t) pImage;

    /* Copy section table */
    status = CopySectionTable(pImage, pNtHeaders, lpData, dwSize, imageSize);
    if (!NT_SUCCESS(status))
        return status;

    /* Adjust base address of imported data */
    const ptrdiff_t locationDelta = pImage - (PUCHAR) lpNtHeaders->OptionalHeader.ImageBase;

    if (locationDelta != 0) {
        PerformBaseRelocation(pImage, pNtHeaders, locationDelta);
    }

    /* Adjust function table of imports */
    status = BuildImportTable(pImage, pNtHeaders);
    if (!NT_SUCCESS(status))
        return status;

    /* Check entry point */
    if (pNtHeaders->OptionalHeader.AddressOfEntryPoint == 0)
        return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS LoadModuleFromMemory(PLOADEDMODULE pModule, const PUCHAR lpData, DWORD dwSize)
{
    NTSTATUS status;

    /* Check header */
    if (!IsPEHeaderValid(lpData, dwSize))
        return STATUS_INVALID_IMAGE_FORMAT;

    const PIMAGE_NT_HEADERS pNtHeaders = GetModuleNtHeaders(lpData);
    const DWORD imageSize = MAX_ALIGNED(pNtHeaders->OptionalHeader.SizeOfImage, PAGE_SIZE);

#ifdef FORT_DEBUG
    LOG("Loader Module: data=%p size=%d imageSize=%d\n", lpData, dwSize, imageSize);
#endif

    /* Allocate the region */
    PUCHAR pImage = fort_mem_exec_alloc(imageSize, FORT_LOADER_POOL_TAG);
    if (pImage == NULL)
        return STATUS_NO_MEMORY;

#ifdef FORT_DEBUG
    LOG("Loader Module: image=%p\n", pImage);
#endif

    status = InitializeModuleImage(pImage, pNtHeaders, lpData, dwSize, imageSize);

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
