/* Fort Firewall Driver Image Handling: Memory Module Loader: Libraries Importer */

#include "fortmm_imp.h"

#if defined(FORT_DRIVER)
#    define IMP_LOG(...)                                                                           \
        moduleImp->dbgPrintEx(DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL, "FORT: " __VA_ARGS__)
#else
#    define IMP_LOG(...)
#endif

static LPCSTR GetModuleOtherName(StrICmpProc strICmp, LPCSTR name)
{
    if (strICmp(name, "ntoskrnl.exe") == 0) {
        return "ntkrnlpa.exe";
    }

    if (strICmp(name, "hal.dll") == 0) {
        return "halmacpi.dll";
    }
    if (strICmp(name, "halmacpi.dll") == 0) {
        return "halacpi.dll";
    }

    return NULL;
}

static NTSTATUS GetModuleInfoFallback(
        PFORT_MODULE_IMP moduleImp, PLOADEDMODULE pModule, LPCSTR name)
{
    NTSTATUS status;

    const DWORD modulesCount = moduleImp->modulesCount;
    PAUX_MODULE_EXTENDED_INFO modules = moduleImp->modules;

    GetModuleInfoProc getModuleInfo = moduleImp->getModuleInfo;

#if defined(FORT_WIN7_COMPAT)
    StrICmpProc strICmp = moduleImp->strICmp;
#endif

    for (;;) {
        status = getModuleInfo(pModule, name, modules, modulesCount);

#if defined(FORT_WIN7_COMPAT)
        if (!NT_SUCCESS(status)) {
            name = GetModuleOtherName(strICmp, name);
            if (name != NULL)
                continue;
        }
#endif

        break;
    }

    return status;
}

static FARPROC ModuleGetProcAddressFallback(
        PFORT_MODULE_IMP moduleImp, PLOADEDMODULE pModule, LPCSTR funcName)
{
    ModuleGetProcAddressProc moduleGetProcAddress = moduleImp->moduleGetProcAddress;

    PLOADEDMODULE forwardModule = moduleImp->forwardModule;
    if (forwardModule != NULL) {
        FARPROC func = moduleGetProcAddress(forwardModule, funcName);
        if (func != NULL) {
#ifdef FORT_DEBUG
            IMP_LOG("Loader Module: Import forwarded: %s\n", funcName);
#endif
            return func;
        }
    }

    return moduleGetProcAddress(pModule, funcName);
}

static NTSTATUS BuildImportTableEntriesBegin(
        PFORT_MODULE_IMP moduleImp, PLOADEDMODULE pModule, PIMAGE_NT_HEADERS pHeaders)
{
    UNUSED(pModule);
    UNUSED(pHeaders);

    PLOADEDMODULE kernelModule = &moduleImp->kernelModule;
    {
        const BOOL isWindows7 = (moduleImp->osMajorVersion == 6 && moduleImp->osMinorVersion == 1);

        if (!isWindows7) {
            moduleImp->getModuleInfoFallback(moduleImp, kernelModule, "ntoskrnl.exe");
        }
    }

    return STATUS_SUCCESS;
}

static void BuildImportTableEntriesEnd(PFORT_MODULE_IMP moduleImp, NTSTATUS status)
{
    UNUSED(moduleImp);
    UNUSED(status);
}

static void BuildImportTableLibraryBegin(PFORT_MODULE_IMP moduleImp, LPCSTR libName)
{
    moduleImp->forwardModule = NULL;

    PLOADEDMODULE kernelModule = &moduleImp->kernelModule;
    if (kernelModule->codeBase != NULL && moduleImp->strICmp(libName, "hal.dll") == 0) {
        /* Functions of HAL.dll are exported from kernel on Windows 8+ */
        IMP_LOG("Loader Module: Forward to kernel: %s\n", libName);
        moduleImp->forwardModule = kernelModule;
    }
}

FORT_API void InitModuleImporter(
        PFORT_MODULE_IMP moduleImp, PAUX_MODULE_EXTENDED_INFO modules, DWORD modulesCount)
{
    RtlZeroMemory(moduleImp, sizeof(FORT_MODULE_IMP));

    /* Get OS version */
    {
        RTL_OSVERSIONINFOW osvi;
        RtlZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        RtlGetVersion(&osvi);

        moduleImp->osMajorVersion = osvi.dwMajorVersion;
        moduleImp->osMinorVersion = osvi.dwMinorVersion;
        moduleImp->osBuildNumber = osvi.dwBuildNumber;
        moduleImp->osPlatformId = osvi.dwPlatformId;
    }

    moduleImp->modulesCount = modulesCount;
    moduleImp->modules = modules;

    moduleImp->dbgPrintEx = DbgPrintEx;
    moduleImp->strICmp = _stricmp;

    moduleImp->getModuleInfo = GetModuleInfo;
    moduleImp->getModuleInfoFallback = GetModuleInfoFallback;
    moduleImp->moduleGetProcAddress = ModuleGetProcAddress;
    moduleImp->moduleGetProcAddressFallback = ModuleGetProcAddressFallback;

    moduleImp->buildImportTableEntriesBegin = BuildImportTableEntriesBegin;
    moduleImp->buildImportTableEntriesEnd = BuildImportTableEntriesEnd;

    moduleImp->buildImportTableLibraryBegin = BuildImportTableLibraryBegin;
}

NTSTATUS DriverImportsSetup(PFORT_MODULE_IMP moduleImp)
{
    moduleImp->getModuleInfoFallback = GetModuleInfoFallback;
    moduleImp->moduleGetProcAddressFallback = ModuleGetProcAddressFallback;

    moduleImp->buildImportTableEntriesBegin = BuildImportTableEntriesBegin;
    moduleImp->buildImportTableEntriesEnd = BuildImportTableEntriesEnd;

    moduleImp->buildImportTableLibraryBegin = BuildImportTableLibraryBegin;

    return STATUS_SUCCESS;
}
