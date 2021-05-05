#include "appinfoutil.h"

#include <QDir>
#include <QPixmap>

#include <comdef.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <shellapi.h>

#include "../util/fileutil.h"
#include "../util/iconcache.h"
#include "appinfo.h"

// Defined in qpixmap_win.cpp
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);

namespace {

QPixmap pixmapFromImageList(int iImageList, const SHFILEINFO &info)
{
    QPixmap result;

    IImageList *imageList;
    HICON hIcon;

    if (SUCCEEDED(SHGetImageList(iImageList, IID_IImageList, reinterpret_cast<void **>(&imageList)))
            && SUCCEEDED(imageList->GetIcon(info.iIcon, ILD_TRANSPARENT, &hIcon))) {
        result = qt_pixmapFromWinHICON(hIcon);
        DestroyIcon(hIcon);
    }

    return result;
}

QPixmap extractShellIcon(const QString &appPath)
{
    const wchar_t *appPathW = (LPCWSTR) appPath.utf16();

    const UINT flags = SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES;

    QPixmap pixmap;

    SHFILEINFOW info;
    ZeroMemory(&info, sizeof(SHFILEINFOW));

    const HRESULT hr = SHGetFileInfoW(appPathW, 0, &info, sizeof(SHFILEINFOW), flags);
    if (hr != 0) {
        pixmap = pixmapFromImageList(SHIL_EXTRALARGE, info);
    }

    return pixmap;
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

    QScopedArrayPointer<BYTE> infoBuf(new BYTE[infoSize]);
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

    const QString productName = extractInfoText(infoData, langInfo, L"ProductName");
    appInfo.productName = productName;

    const QString fileDescription = extractInfoText(infoData, langInfo, L"FileDescription");
    appInfo.fileDescription = !fileDescription.isEmpty()
            ? fileDescription
            : (!productName.isEmpty() ? productName : FileUtil::fileName(appPath));

    return true;
}

}

namespace AppInfoUtil {

bool getInfo(const QString &appPath, AppInfo &appInfo)
{
    if (appPath.isEmpty())
        return false;

    if (FileUtil::isSystemApp(appPath)) {
        appInfo.fileDescription = FileUtil::systemApp();
        return true;
    }

    if (!extractVersionInfo(appPath, appInfo))
        return false;

    // File modification time
    appInfo.fileModTime = FileUtil::fileModTime(appPath);

    return true;
}

QImage getIcon(const QString &appPath)
{
    if (appPath.isEmpty())
        return {};

    if (FileUtil::isSystemApp(appPath)) {
        const auto pixmap = IconCache::file(":/images/windows-48.png");
        return pixmap.toImage();
    }

    return extractShellIcon(appPath).toImage();
}

void initThread()
{
    CoInitialize(nullptr);
}

void doneThread()
{
    CoUninitialize();
}

}
