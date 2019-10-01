#include "apputil.h"

#include <QDateTime>
#include <QDir>
#include <QPixmap>

#include <comdef.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <shellapi.h>

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

    const HRESULT hr = SHGetFileInfoW(appPathW, 0, &info,
                                      sizeof(SHFILEINFOW), flags);
    if (SUCCEEDED(hr)) {
        pixmap = pixmapFromImageList(SHIL_EXTRALARGE, info);
    }

    return pixmap;
}

QString extractInfoText(LPVOID infoData, const WORD *langInfo,
                        const WCHAR *name)
{
    WCHAR verStrName[128];
    wsprintfW(verStrName, L"\\StringFileInfo\\%04x%04x\\%s",
              langInfo[0], langInfo[1], name);

    WCHAR *content;
    UINT len;
    if (VerQueryValueW(infoData, verStrName,
                       (LPVOID *) &content, &len)
            && len > 1) {
        return QString::fromWCharArray(content, int(len) - 1);
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

        appInfo.productVersion = QString("%1.%2.%3.%4").arg(
                    QString::number(leftMost), QString::number(secondLeft),
                    QString::number(secondRight), QString::number(rightMost));

        if (rightMost == 0) {
            appInfo.productVersion.chop(2);
        }
    }

    // Language info
    WORD *langInfo;
    if (!VerQueryValueA(infoData, "\\VarFileInfo\\Translation",
                        (LPVOID *) &langInfo, (PUINT) &dummy))
        return false;

    // Texts
    appInfo.productName = extractInfoText(infoData, langInfo, L"ProductName");
    appInfo.companyName = extractInfoText(infoData, langInfo, L"CompanyName");
    appInfo.fileDescription = extractInfoText(infoData, langInfo, L"FileDescription");

    return true;
}

bool isSystemApp(const QString &appPath)
{
    return appPath.compare("System", Qt::CaseInsensitive) == 0;
}

}

bool AppUtil::getInfo(const QString &appPath, AppInfo &appInfo)
{
    if (appPath.isEmpty())
        return false;

    if (isSystemApp(appPath)) {
        appInfo.fileDescription = appPath;
        return true;
    }

    // File modification time
    appInfo.fileModTime = getModTime(appPath);

    return extractVersionInfo(appPath, appInfo);
}

QImage AppUtil::getIcon(const QString &appPath)
{
    if (appPath.isEmpty())
        return {};

    if (isSystemApp(appPath)) {
        return QImage(":/images/windows-48.png");
    }

    return extractShellIcon(appPath)
            .toImage();
}

qint64 AppUtil::getModTime(const QString &appPath)
{
    QFileInfo fi(appPath);
    return fi.lastModified().toMSecsSinceEpoch();
}
