/* Fort Firewall Driver Image Handling: Memory Module Loader */

#include "fortmm.h"

#if defined(FORT_DRIVER)
#    include "ntimage.h"

#    define IS_INTRESOURCE(_r) ((((ULONG_PTR) (_r)) >> 16) == 0)
#else
#    define PAGE_SIZE 0x1000
#endif

#define DEFAULT_LANGUAGE MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)

#define ARCHITECTURE_TYPE_X86 0x00000000
#define ARCHITECTURE_TYPE_X64 0x00000001

// MIN/MAX of address aligned
#define MIN_ALIGNED(address, alignment) (LPVOID)((uintptr_t) (address) & ~((alignment) -1))
#define MAX_ALIGNED(value, alignment)   (((value) + (alignment) -1) & ~((alignment) -1))

// Section data from file header
typedef struct _SECTIONDATA
{
    LPVOID pAddress;
    LPVOID pAlignedAddress;
    uintptr_t Size;
    DWORD dwCharacteristics;
    BOOL fLast;
} SECTIONDATA, *PSECTIONDATA;

typedef BOOL(WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

static NTSTATUS CopySectionTable(
        PUCHAR pData, size_t size, PIMAGE_NT_HEADERS pNtheaders, PLOADEDMODULE pLoadedModule)
{
    const PUCHAR codeBase = pLoadedModule->pCodeBase;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pLoadedModule->pHeaders);

    for (int i = 0; i < pLoadedModule->pHeaders->FileHeader.NumberOfSections; ++i, ++section) {
        if (section->SizeOfRawData == 0) {
            /* Section doesn't contain data in the dll itself, but may define uninitialized data. */
            const INT sectionSize = pNtheaders->OptionalHeader.SectionAlignment;
            if (sectionSize > 0) {
                PUCHAR dest = (unsigned char *) VirtualAlloc(codeBase + section->VirtualAddress,
                        sectionSize, MEM_COMMIT, PAGE_READWRITE);
                if (dest == NULL)
                    return STATUS_NO_MEMORY;

                // Always use position from file to support alignments smaller
                // than page size.
                dest = codeBase + section->VirtualAddress;
                section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;
                RtlZeroMemory(dest, sectionSize);
            }
        } else {
            if (size < (SIZE_T) section->PointerToRawData + section->SizeOfRawData)
                return STATUS_INVALID_IMAGE_FORMAT;

            /* Commit memory block and copy data from dll */
            PUCHAR dest = (unsigned char *) VirtualAlloc(codeBase + section->VirtualAddress,
                    section->SizeOfRawData, MEM_COMMIT, PAGE_READWRITE);
            if (dest == NULL)
                return STATUS_NO_MEMORY;

            /* Always use position from file to support alignments smaller than page size. */
            dest = codeBase + section->VirtualAddress;
            RtlCopyMemory(dest, pData + section->PointerToRawData, section->SizeOfRawData);

            /* NOTE: On 64bit systems we truncate to 32bit here but expand
             * again later when "PhysicalAddress" is used.
             */
            section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;
        }
    }

    return STATUS_SUCCESS;
}

/*
 * The DLL's preferred load address conflicts with memory that's already in use
 * so we need to 'rebases' the DLL by loading it at a different address that does
 * not overlap and then adjust all addresses.
 */
static NTSTATUS PerformBaseRelocation(PLOADEDMODULE pLoadedModule, ptrdiff_t overlapDelta)
{
    PIMAGE_DATA_DIRECTORY directory =
            &(pLoadedModule)
                     ->pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (directory->Size == 0) {
        return (overlapDelta == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    const PUCHAR codeBase = pLoadedModule->pCodeBase;
    PIMAGE_BASE_RELOCATION relocation =
            (PIMAGE_BASE_RELOCATION) (codeBase + directory->VirtualAddress);

    for (; relocation->VirtualAddress > 0;) {
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
                *patchAddrHL += (DWORD) overlapDelta;
                break;

#ifdef _WIN64
            case IMAGE_REL_BASED_DIR64:
                ULONGLONG *patchAddr64 = (PULONGLONG) (dest + offset);
                *patchAddr64 += (ULONGLONG) overlapDelta;
                break;
#endif

            default:
                break;
            }
        }

        /* Advance to next relocation block */
        relocation = (PIMAGE_BASE_RELOCATION) (((PCHAR) relocation) + relocation->SizeOfBlock);
    }

    return STATUS_SUCCESS;
}

/* Build the import address table. */
static NTSTATUS BuildImportTable(PLOADEDMODULE pLoadedModule)
{
    PIMAGE_DATA_DIRECTORY directory =
            &(pLoadedModule)->pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (directory->Size == 0)
        return STATUS_SUCCESS;

    const PUCHAR codeBase = pLoadedModule->pCodeBase;
    PIMAGE_IMPORT_DESCRIPTOR importDesc =
            (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);

    for (; importDesc != NULL && importDesc->Name != 0; ++importDesc) {
        const HINSTANCE handle = LoadLibraryA((LPCSTR) (codeBase + importDesc->Name));
        if (handle == NULL)
            return STATUS_DLL_NOT_FOUND;

        HINSTANCE *const tmp = (HINSTANCE *) realloc(pLoadedModule->pModules,
                ((SIZE_T) pLoadedModule->nModules + 1) * (sizeof(HINSTANCE)));
        if (tmp == NULL)
            return STATUS_NO_MEMORY;

        pLoadedModule->pModules = tmp;
        pLoadedModule->pModules[pLoadedModule->nModules++] = handle;

        const DWORD originalFirstThunk = (importDesc->OriginalFirstThunk != 0)
                ? importDesc->OriginalFirstThunk
                : importDesc->FirstThunk;
        uintptr_t *thunkRef = (uintptr_t *) (codeBase + originalFirstThunk);
        FARPROC *funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);

        for (; *thunkRef; ++thunkRef, ++funcRef) {
            if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
                *funcRef = (FARPROC) GetProcAddress(handle, (LPCSTR) IMAGE_ORDINAL(*thunkRef));
            } else {
                PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME) (codeBase + (*thunkRef));
                *funcRef = (FARPROC) GetProcAddress(handle, (LPCSTR) &thunkData->Name);
            }

            if (*funcRef == 0) {
                FreeLibrary(handle);
                return STATUS_PROCEDURE_NOT_FOUND;
            }
        }
    }

    return STATUS_SUCCESS;
}

/* Determine section size. */
static DWORD GetRealSectionSize(PLOADEDMODULE pModule, PIMAGE_SECTION_HEADER section)
{
    if (section->SizeOfRawData != 0)
        return section->SizeOfRawData;

    if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
        return pModule->pHeaders->OptionalHeader.SizeOfInitializedData;
    } else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
        return pModule->pHeaders->OptionalHeader.SizeOfUninitializedData;
    }
    return 0;
}

/* VirtualProtect section based on Characteristics flags. */
static BOOL ProtectSection(PLOADEDMODULE pModule, PSECTIONDATA SectionData)
{
    if (SectionData->Size == 0)
        return TRUE;

    /* See if section is not needed any more and can be safely freed */
    if (SectionData->dwCharacteristics & IMAGE_SCN_MEM_DISCARDABLE) {
        if (SectionData->pAddress == SectionData->pAlignedAddress
                && (SectionData->fLast
                        || pModule->pHeaders->OptionalHeader.SectionAlignment == pModule->dwPageSize
                        || (SectionData->Size % pModule->dwPageSize) == 0)) {
            /* Only allowed to decommit whole pages */
#pragma warning(push)
#pragma warning(disable : 6250)
            VirtualFree(SectionData->pAddress, SectionData->Size, MEM_DECOMMIT);
#pragma warning(pop)
        }
        return TRUE;
    }

    /* Determine protection flags based on Characteristics */
    if (SectionData->dwCharacteristics & IMAGE_SCN_CNT_CODE) {
        SectionData->dwCharacteristics |= IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
    }

    /* We rotate the upper 3 important bits down so the resulting value is in the range 0-7.
     * Meaning of bits: 1: execute, 2: read, 4: write
     */
    DWORD protection;
    switch ((DWORD) SectionData->dwCharacteristics >> (32 - 3)) {
    case 1:
        protection = PAGE_EXECUTE;
        break;
    case 0: /* case 0: what does it mean? */
    case 2:
        protection = PAGE_READONLY;
        break;
    case 3:
        protection = PAGE_EXECUTE_READ;
        break;
    case 4:
    case 6:
        protection = PAGE_READWRITE;
        break;
    case 5:
    default:
        protection = PAGE_EXECUTE_READWRITE;
        break;
    }

    if (SectionData->dwCharacteristics & IMAGE_SCN_MEM_NOT_CACHED) {
        protection |= PAGE_NOCACHE;
    }

    /* Change memory access flags */
    DWORD oldProtection;
    if (VirtualProtect(SectionData->pAddress, SectionData->Size, protection, &oldProtection) == 0) {
        return FALSE;
    }

    return TRUE;
}

/* Set protection of memory pages. */
static BOOL ProtectSections(PLOADEDMODULE pModule)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pModule->pHeaders);
    const uintptr_t imageOffset =
#ifdef _WIN64
            (pModule->pHeaders->OptionalHeader.ImageBase & 0xffffffff00000000);
#else
            0;
#endif

    SECTIONDATA SectionData;
    SectionData.pAddress = (PVOID) ((uintptr_t) section->Misc.PhysicalAddress | imageOffset);
    SectionData.pAlignedAddress = MIN_ALIGNED(SectionData.pAddress, pModule->dwPageSize);
    SectionData.Size = GetRealSectionSize(pModule, section);
    SectionData.dwCharacteristics = section->Characteristics;
    SectionData.fLast = FALSE;

    ++section;

    /* Loop through all sections and change access flags */
    for (int i = 1; i < pModule->pHeaders->FileHeader.NumberOfSections; ++i, ++section) {
        const PVOID SectionAddress =
                (PVOID) ((uintptr_t) section->Misc.PhysicalAddress | imageOffset);
        const PVOID AlignedAddress = MIN_ALIGNED(SectionAddress, pModule->dwPageSize);
        const DWORD SectionSize = GetRealSectionSize(pModule, section);

        if (SectionData.pAlignedAddress == AlignedAddress
                || (uintptr_t) SectionData.pAddress + SectionData.Size
                        > (uintptr_t) AlignedAddress) {
            /* Section shares page with previous section */
            if ((section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0
                    || (SectionData.dwCharacteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0) {
                SectionData.dwCharacteristics =
                        (SectionData.dwCharacteristics | section->Characteristics)
                        & ~IMAGE_SCN_MEM_DISCARDABLE;
            } else {
                SectionData.dwCharacteristics |= section->Characteristics;
            }
            SectionData.Size =
                    (((uintptr_t) SectionAddress) + SectionSize) - (uintptr_t) SectionData.pAddress;
            continue;
        }

        if (!ProtectSection(pModule, &SectionData)) {
            return FALSE;
        }
        SectionData.pAddress = SectionAddress;
        SectionData.pAlignedAddress = AlignedAddress;
        SectionData.Size = SectionSize;
        SectionData.dwCharacteristics = section->Characteristics;
    }

    SectionData.fLast = TRUE;
    if (!ProtectSection(pModule, &SectionData)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL IsPEHeaderValid(__in PVOID lpData, __in DWORD dwSize)
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

    return TRUE;
}

static NTSTATUS InitializeModuleImage(__in PLOADEDMODULE pModule, __in_bcount(dwSize) PVOID lpData,
        __in DWORD dwSize, __in PUCHAR pImage, __in PIMAGE_DOS_HEADER pDosHeader,
        __in PIMAGE_NT_HEADERS pNtHeaders)
{
    NTSTATUS status;

    pModule->pCodeBase = pImage;
    pModule->dwPageSize = PAGE_SIZE;

    /* Commit memory for headers */
    PUCHAR pHeaders = (PUCHAR) VirtualAlloc(
            pImage, pNtHeaders->OptionalHeader.SizeOfHeaders, MEM_COMMIT, PAGE_READWRITE);
    if (pHeaders == NULL)
        return STATUS_NO_MEMORY;

    /* Copy PE header to code */
    RtlCopyMemory(pHeaders, pDosHeader, pNtHeaders->OptionalHeader.SizeOfHeaders);
    pModule->pHeaders =
            (PIMAGE_NT_HEADERS) & ((const unsigned char *) (pHeaders))[pDosHeader->e_lfanew];

    /* Update position in case we didn't get preferred base */
    pModule->pHeaders->OptionalHeader.ImageBase = (uintptr_t) pImage;

    /* Copy section table */
    status = CopySectionTable((PUCHAR) lpData, dwSize, pNtHeaders, pModule);
    if (!NT_SUCCESS(status))
        return status;

    /* Adjust base address of imported data */
    const ptrdiff_t locationDelta = (ptrdiff_t) pModule->pHeaders->OptionalHeader.ImageBase
            - pNtHeaders->OptionalHeader.ImageBase;
    if (locationDelta != 0) {
        pModule->fRelocated = PerformBaseRelocation(pModule, locationDelta);
    } else {
        pModule->fRelocated = TRUE;
    }

    /* Adjust function table of imports */
    status = BuildImportTable(pModule);
    if (!NT_SUCCESS(status))
        return status;

    /* Mark memory pages depending on characteristics of the section headers */
    status = ProtectSections(pModule);
    if (!NT_SUCCESS(status))
        return status;

    /* Thread Local Storage (TLS) callbacks are executed BEFORE the main loading */
    PIMAGE_DATA_DIRECTORY directory =
            &(pModule->pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS]);
    if (directory->VirtualAddress != 0) {
        PIMAGE_TLS_DIRECTORY tls =
                (PIMAGE_TLS_DIRECTORY) (pModule->pCodeBase + directory->VirtualAddress);
        PIMAGE_TLS_CALLBACK *callback = (PIMAGE_TLS_CALLBACK *) tls->AddressOfCallBacks;

        if (callback) {
            while (*callback) {
                (*callback)((PVOID) pModule->pCodeBase, DLL_PROCESS_ATTACH, NULL);
                callback++;
            }
        }
    }

    /* Get entry point and call DLL_PROCESS_ATTACH */
    if (pModule->pHeaders->OptionalHeader.AddressOfEntryPoint != 0) {
        DllEntryProc DllEntry = (DllEntryProc) (PVOID) (pModule->pCodeBase
                + pModule->pHeaders->OptionalHeader.AddressOfEntryPoint);

        /* Notify library about attaching to process */
        const BOOL successful = (*DllEntry)((HINSTANCE) pModule->pCodeBase, DLL_PROCESS_ATTACH, 0);
        if (!successful)
            return STATUS_DLL_INIT_FAILED;

        pModule->fInitialized = TRUE;
    }

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS LoadModuleFromMemory(
        __in PLOADEDMODULE pModule, __in_bcount(dwSize) PVOID lpData, __in DWORD dwSize)
{
    NTSTATUS status;

    if (!IsPEHeaderValid(lpData, dwSize))
        return STATUS_INVALID_IMAGE_FORMAT;

    /* Check header for valid signatures */
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) lpData;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS) & ((PUCHAR) lpData)[pDosHeader->e_lfanew];

    SIZE_T lastSectionEnd = 0;
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);
    for (DWORD i = 0; i < pNtHeaders->FileHeader.NumberOfSections; ++i, ++pSection) {
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

    /* Reserve pages at specified image base. */
    PUCHAR pImage = (PUCHAR) VirtualAlloc((PVOID) pNtHeaders->OptionalHeader.ImageBase, imageSize,
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (pImage == NULL) {
        /* Allow system to determine where to allocate the region */
        pImage = (PUCHAR) VirtualAlloc(NULL, imageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (pImage == NULL)
            return STATUS_NO_MEMORY;
    }

    status = InitializeModuleImage(pModule, lpData, dwSize, pImage, pDosHeader, pNtHeaders);
    if (!NT_SUCCESS(status)) {
        VirtualFree(pImage, 0, MEM_RELEASE);
    }

    return status;
}

/* Free all resources allocated for a loaded module */
FORT_API void UnloadModule(__in PLOADEDMODULE pModule)
{
    if (pModule->fInitialized) {
        /* Tell library to detach from process */
        DllEntryProc DllEntry = (DllEntryProc) (PVOID) (pModule->pCodeBase
                + pModule->pHeaders->OptionalHeader.AddressOfEntryPoint);
        (*DllEntry)((HINSTANCE) pModule->pCodeBase, DLL_PROCESS_DETACH, 0);
    }

    if (pModule->pCodeBase != NULL) {
        /* Release memory of module */
        VirtualFree(pModule->pCodeBase, 0, MEM_RELEASE);
    }
}

/* Retrieve address of an exported function from our modules DLL. */
FORT_API FARPROC ModuleGetProcAddress(__in PLOADEDMODULE pModule, __in_z_opt LPCSTR FuncName)
{
    DWORD idx = 0;
    PIMAGE_EXPORT_DIRECTORY exports;
    PIMAGE_DATA_DIRECTORY directory;

    directory = &(pModule->pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    if (directory->Size == 0)
        return NULL; /* no export table found */

    exports = (PIMAGE_EXPORT_DIRECTORY) (pModule->pCodeBase + directory->VirtualAddress);
    if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0)
        return NULL; /* Our modules must export 3 functions. */

    if (HIWORD(FuncName) == 0) {
        /* Load function by ordinal value */
        if (LOWORD(FuncName) < exports->Base)
            return NULL;

        idx = LOWORD(FuncName) - exports->Base;
    } else {
        /* Search function name in list of exported names */
        BOOL found = FALSE;
        DWORD *nameRef = (DWORD *) (pModule->pCodeBase + exports->AddressOfNames);
        WORD *ordinal = (WORD *) (pModule->pCodeBase + exports->AddressOfNameOrdinals);

        for (DWORD i = 0; i < exports->NumberOfNames; ++i, ++nameRef, ++ordinal) {
            if (strcmp(FuncName, (const char *) (pModule->pCodeBase + (*nameRef))) == 0) {
                idx = *ordinal;
                found = TRUE;
                break;
            }
        }

        if (!found)
            return NULL; /* exported symbol not found */
    }

    if (idx > exports->NumberOfFunctions)
        return NULL; /* name <-> ordinal number don't match */

    /* AddressOfFunctions contains the RVAs to the "real" functions */
    return (FARPROC) (PVOID) (pModule->pCodeBase
            + (*(DWORD *) (pModule->pCodeBase + exports->AddressOfFunctions + ((SIZE_T) idx * 4))));
}

static PIMAGE_RESOURCE_DIRECTORY_ENTRY SearchResourceEntry(
        __in void *root, __in PIMAGE_RESOURCE_DIRECTORY resources, __in_opt LPCTSTR key)
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY entries = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (resources + 1);
    PIMAGE_RESOURCE_DIRECTORY_ENTRY result = NULL;
    DWORD start;
    DWORD end;
    DWORD middle;

    /* Entries are stored as ordered list of named entries,
     * followed by an ordered list of id entries - we can do
     * a binary search to find faster...
     */
    if (IS_INTRESOURCE(key)) {
        const WORD check = (WORD) (uintptr_t) key;
        start = resources->NumberOfNamedEntries;
        end = start + resources->NumberOfIdEntries;

        while (end > start) {
            middle = (start + end) >> 1;
            const WORD entryName = (WORD) entries[middle].Name;
            if (check < entryName) {
                end = (end != middle ? middle : middle - 1);
            } else if (check > entryName) {
                start = (start != middle ? middle : middle + 1);
            } else {
                result = &entries[middle];
                break;
            }
        }
    } else {
        const int searchKeyLen = (int) wcslen((wchar_t *) key);
        LPCWSTR searchKey = (LPCWSTR) key;
        start = 0;
        end = resources->NumberOfNamedEntries;
        while (end > start) {
            PIMAGE_RESOURCE_DIR_STRING_U resourceString;
            middle = (start + end) >> 1;
            resourceString = (PIMAGE_RESOURCE_DIR_STRING_U) (((char *) root)
                    + (entries[middle].Name & 0x7FFFFFFF));

            int cmp = _wcsnicmp(searchKey, resourceString->NameString, resourceString->Length);
            if (cmp == 0) {
                // Handle partial match
                cmp = searchKeyLen - (int) resourceString->Length;
            }

            if (cmp < 0) {
                end = (middle != end ? middle : middle - 1);
            } else if (cmp > 0) {
                start = (middle != start ? middle : middle + 1);
            } else {
                result = &entries[middle];
                break;
            }
        }
    }

    return result;
}

FORT_API PVOID ModuleFindResource(
        __in_opt PLOADEDMODULE module, __in_z LPCTSTR name, __in_z_opt LPCTSTR type)
{
    const PUCHAR codeBase = ((PLOADEDMODULE) module)->pCodeBase;

    PIMAGE_DATA_DIRECTORY directory =
            &(module)->pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];

    if (directory->Size == 0)
        return NULL; /* no resource table found */

    /* resources are stored as three-level tree
     * - first node is the type
     * - second node is the name
     * - third node is the language
     */
    PIMAGE_RESOURCE_DIRECTORY rootResources =
            (PIMAGE_RESOURCE_DIRECTORY) (codeBase + directory->VirtualAddress);
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundType =
            SearchResourceEntry(rootResources, rootResources, type);
    if (foundType == NULL)
        return NULL; /* no resource type found */

    PIMAGE_RESOURCE_DIRECTORY typeResources = (PIMAGE_RESOURCE_DIRECTORY) (codeBase
            + directory->VirtualAddress + (foundType->OffsetToData & 0x7fffffff));
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundName =
            SearchResourceEntry(rootResources, typeResources, name);
    if (foundName == NULL)
        return NULL; /* no resource name found */

    const WORD language = DEFAULT_LANGUAGE;

    PIMAGE_RESOURCE_DIRECTORY nameResources = (PIMAGE_RESOURCE_DIRECTORY) (codeBase
            + directory->VirtualAddress + (foundName->OffsetToData & 0x7fffffff));
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundLanguage =
            SearchResourceEntry(rootResources, nameResources, (LPCTSTR) (uintptr_t) language);
    if (foundLanguage == NULL) {
        /* requested language not found, use first available */
        if (nameResources->NumberOfIdEntries == 0)
            return NULL; /* no resource language found */

        foundLanguage = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (nameResources + 1);
    }

    return (codeBase + directory->VirtualAddress + (foundLanguage->OffsetToData & 0x7fffffff));
}

FORT_API DWORD ModuleSizeofResource(__in PLOADEDMODULE module, __in_opt PVOID resource)
{
    if (resource == NULL) {
        return 0;
    }

    UNREFERENCED_PARAMETER(module);
    const PIMAGE_RESOURCE_DATA_ENTRY entry = (PIMAGE_RESOURCE_DATA_ENTRY) resource;

    return entry->Size;
}

FORT_API LPVOID ModuleLoadResource(__in PLOADEDMODULE module, __in PVOID resource)
{
    if (resource == NULL) {
        return NULL;
    }

    const PUCHAR codeBase = ((PLOADEDMODULE) module)->pCodeBase;
    const PIMAGE_RESOURCE_DATA_ENTRY entry = (PIMAGE_RESOURCE_DATA_ENTRY) resource;

    return codeBase + entry->OffsetToData;
}
