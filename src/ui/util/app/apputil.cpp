#include "apputil.h"

#include <QDir>

#include <comdef.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <shellapi.h>

// Defined in qpixmap_win.cpp
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);

namespace {

static QPixmap pixmapFromImageList(int iImageList, const SHFILEINFO &info)
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

static QPixmap extractShellIcon(const QString &appPath)
{
    const QString nativePath = QDir::toNativeSeparators(appPath);
    const wchar_t *nativePathW = reinterpret_cast<const wchar_t *>(nativePath.utf16());

    const UINT flags = SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES;

    QPixmap pixmap;

    SHFILEINFOW info;
    ZeroMemory(&info, sizeof(SHFILEINFOW));

    const HRESULT hr = SHGetFileInfoW(nativePathW, 0, &info,
                                      sizeof(SHFILEINFOW), flags);
    if (SUCCEEDED(hr)) {
        pixmap = pixmapFromImageList(SHIL_JUMBO, info);
    }

    return pixmap;
}

}

AppUtil::AppUtil(QObject *parent) :
    QObject(parent)
{
}

QPixmap AppUtil::getIcon(const QString &appPath)
{
    return extractShellIcon(appPath);
}
