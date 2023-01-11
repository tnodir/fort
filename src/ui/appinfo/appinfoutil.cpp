#include "appinfoutil.h"

#include <QImage>
#include <QVarLengthArray>

#include <comdef.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <shellapi.h>

#include <serviceinfo/serviceinfomanager.h>
#include <util/fileutil.h>
#include <util/osutil.h>

#include "appinfo.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#    include <QPixmap>
// Defined in qpixmap_win.cpp
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
#endif

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        result = qt_pixmapFromWinHICON(hIcon).toImage();
#else
        result = QImage::fromHICON(hIcon);
#endif
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

QString extractInfoText(LPVOID infoData, const WORD *langInfo, const WCHAR *name)
{
    WCHAR verStrName[128];
    wsprintfW(verStrName, L"\\StringFileInfo\\%04x%04x\\%s", langInfo[0], langInfo[1], name);

    WCHAR *content;
    UINT len;
    if (VerQueryValueW(infoData, verStrName, (LPVOID *) &content, &len) && len > 1) {
        return QString::fromWCharArray(content).trimmed();
    }

    return QString();
}

bool extractVersionInfo(const QString &appPath, AppInfo &appInfo)
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
    {
        VS_FIXEDFILEINFO *ffi;
        if (!VerQueryValueA(infoData, "\\", (LPVOID *) &ffi, (PUINT) &dummy))
            return false;

        const DWORD leftMost = HIWORD(ffi->dwProductVersionMS);
        const DWORD secondLeft = LOWORD(ffi->dwProductVersionMS);
        const DWORD secondRight = HIWORD(ffi->dwProductVersionLS);
        const DWORD rightMost = LOWORD(ffi->dwProductVersionLS);

        appInfo.productVersion =
                QString("%1.%2.%3.%4")
                        .arg(QString::number(leftMost), QString::number(secondLeft),
                                QString::number(secondRight), QString::number(rightMost));

        if (rightMost == 0) {
            appInfo.productVersion.chop(2);
        }
    }

    // Language info
    WORD *langInfo;
    if (!VerQueryValueA(
                infoData, "\\VarFileInfo\\Translation", (LPVOID *) &langInfo, (PUINT) &dummy))
        return false;

    // Texts
    appInfo.companyName = extractInfoText(infoData, langInfo, L"CompanyName");
    appInfo.productName = extractInfoText(infoData, langInfo, L"ProductName");
    appInfo.fileDescription = extractInfoText(infoData, langInfo, L"FileDescription");

    return true;
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

    const bool ok = extractVersionInfo(path, appInfo);

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
