#include "appinfoutil.h"

#include <QImage>
#include <QVarLengthArray>

#include <comdef.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <shellapi.h>

#include <manager/serviceinfomanager.h>
#include <util/fileutil.h>
#include <util/osutil.h>

#include "appinfo.h"

namespace {

struct Wow64FsRedirection
{
    BOOL disabled = false;
    PVOID oldValue = nullptr;
};

inline Wow64FsRedirection disableWow64FsRedirection()
{
    Wow64FsRedirection v;
#if !defined(Q_OS_WIN64)
    v.disabled = Wow64DisableWow64FsRedirection(&v.oldValue);
#endif
    return v;
}

inline void revertWow64FsRedirection(const Wow64FsRedirection &v)
{
#if !defined(Q_OS_WIN64)
    if (v.disabled) {
        Wow64RevertWow64FsRedirection(v.oldValue);
    }
#else
    Q_UNUSED(v);
#endif
}

QImage imageFromImageList(int iImageList, const SHFILEINFO &info)
{
    QImage result;

    IImageList *imageList;
    HICON hIcon;

    if (SUCCEEDED(SHGetImageList(iImageList, IID_IImageList, reinterpret_cast<void **>(&imageList)))
            && SUCCEEDED(imageList->GetIcon(info.iIcon, ILD_TRANSPARENT, &hIcon))) {
        result = QImage::fromHICON(hIcon);
        DestroyIcon(hIcon);
    }

    return result;
}

QImage extractShellIcon(const QString &appPath)
{
    const wchar_t *appPathW = (LPCWSTR) appPath.utf16();

    const UINT flags = SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES;

    QImage result;

    SHFILEINFOW info;
    ZeroMemory(&info, sizeof(SHFILEINFOW));

    const HRESULT hr = SHGetFileInfoW(appPathW, 0, &info, sizeof(SHFILEINFOW), flags);
    if (hr != 0) {
        result = imageFromImageList(SHIL_EXTRALARGE, info);
    }

    return result;
}

QString extractInfoText(LPVOID infoData, const DWORD langInfo, const WCHAR *name)
{
    WCHAR verStrName[128];
    wsprintfW(verStrName, L"\\StringFileInfo\\%08hX\\%s", langInfo, name);

    WCHAR *content;
    UINT len;
    if (VerQueryValueW(infoData, verStrName, (LPVOID *) &content, &len) && len > 1) {
        return QString::fromWCharArray(content).trimmed();
    }

    return QString();
}

QString extractInfoVersion(LPVOID infoData)
{
    UINT dummy;
    VS_FIXEDFILEINFO *ffi;
    if (!VerQueryValueA(infoData, "\\", (LPVOID *) &ffi, &dummy))
        return {};

    DWORD versionMS;
    DWORD versionLS;

    if (ffi->dwFileVersionMS != 0 || ffi->dwFileVersionLS != 0) {
        versionMS = ffi->dwFileVersionMS;
        versionLS = ffi->dwFileVersionLS;
    } else {
        versionMS = ffi->dwProductVersionMS;
        versionLS = ffi->dwProductVersionLS;
    }

    const WORD leftMost = HIWORD(versionMS);
    const WORD secondLeft = LOWORD(versionMS);
    const WORD secondRight = HIWORD(versionLS);
    const WORD rightMost = LOWORD(versionLS);

    QString version = QString("%1.%2.%3.%4")
                              .arg(QString::number(leftMost), QString::number(secondLeft),
                                      QString::number(secondRight), QString::number(rightMost));

    if (rightMost == 0) {
        version.chop(2);
    }

    return version;
}

bool extractProductInfo(const QString &appPath, AppInfo &appInfo)
{
    const wchar_t *appPathW = (LPCWSTR) appPath.utf16();

    const DWORD flags = FILE_VER_GET_NEUTRAL | FILE_VER_GET_PREFETCHED;

    DWORD dummy;
    const DWORD infoSize = GetFileVersionInfoSizeExW(flags, appPathW, &dummy);
    if (infoSize == 0)
        return false;

    QVarLengthArray<BYTE, 4096> infoBuf(infoSize);
    LPVOID infoData = infoBuf.data();

    if (!GetFileVersionInfoExW(flags, appPathW, 0, infoSize, infoData))
        return false;

    // Product Version
    appInfo.productVersion = extractInfoVersion(infoData);

    // Language info
    DWORD langInfoList[] = { 0x040904B0, 0x040904B0, 0x040904E4, 0x04090000 };
    constexpr int langInfoCount = sizeof(langInfoList) / sizeof(DWORD);

    WORD *infoList;
    if (VerQueryValueA(
                infoData, "\\VarFileInfo\\Translation", (LPVOID *) &infoList, (PUINT) &dummy)) {
        langInfoList[0] = (infoList[0] << 16) | infoList[1];
    }

    // Texts
    for (int i = 0; i < langInfoCount; ++i) {
        const DWORD langInfo = langInfoList[i];

        appInfo.companyName = extractInfoText(infoData, langInfo, L"CompanyName");
        appInfo.productName = extractInfoText(infoData, langInfo, L"ProductName");
        appInfo.fileDescription = extractInfoText(infoData, langInfo, L"FileDescription");

        if (!appInfo.fileDescription.isEmpty())
            return true;
    }

    return false;
}

}

namespace AppInfoUtil {

bool getInfo(const QString &appPath, AppInfo &appInfo)
{
    if (appPath.isEmpty())
        return false;

    if (FileUtil::isSystemApp(appPath)) {
        appInfo.fileDescription = FileUtil::systemAppDescription();
        return true;
    }

    QString path = appPath;

    // Service Name: Set real path
    QString serviceName;
    if (FileUtil::isSvcHostService(appPath, serviceName)) {
        path = ServiceInfoManager::getSvcHostServiceDll(serviceName);
        appInfo.altPath = path;
    }

    const auto wow64FsRedir = disableWow64FsRedirection();

    // File modification time
    appInfo.fileModTime = FileUtil::fileModTime(path);

    const bool ok = appInfo.fileModTime.isValid();
    if (ok) {
        extractProductInfo(path, appInfo);
    }

    revertWow64FsRedirection(wow64FsRedir);

    // File description
    if (appInfo.fileDescription.isEmpty()) {
        appInfo.fileDescription =
                !appInfo.productName.isEmpty() ? appInfo.productName : FileUtil::fileName(appPath);
    }

    return ok;
}

QImage getIcon(const QString &appPath)
{
    if (appPath.isEmpty())
        return {};

    if (FileUtil::isSystemApp(appPath)) {
        return QImage(":/icons/windows-48.png");
    }

    const auto wow64FsRedir = disableWow64FsRedirection();

    const QImage result = extractShellIcon(appPath);

    revertWow64FsRedirection(wow64FsRedir);

    return result;
}

void initThread()
{
    CoInitialize(nullptr);
}

void doneThread()
{
    CoUninitialize();
}

bool fileExists(const QString &appPath)
{
    const auto wow64FsRedir = disableWow64FsRedirection();

    const bool res = FileUtil::fileExists(appPath);

    revertWow64FsRedirection(wow64FsRedir);

    return res;
}

QDateTime fileModTime(const QString &appPath)
{
    if (appPath.isEmpty() || FileUtil::isSystemApp(appPath))
        return {};

    const auto wow64FsRedir = disableWow64FsRedirection();

    const QDateTime res = FileUtil::fileModTime(appPath);

    revertWow64FsRedirection(wow64FsRedir);

    return res;
}

bool openFolder(const QString &appPath)
{
    const auto wow64FsRedir = disableWow64FsRedirection();

    const bool res = OsUtil::openFolder(appPath);

    revertWow64FsRedirection(wow64FsRedir);

    return res;
}

}
