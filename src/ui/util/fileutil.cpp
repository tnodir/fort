#include "fileutil.h"

#include <QDir>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

FileUtil::FileUtil(QObject *parent) :
    QObject(parent)
{
    Q_ASSERT(sizeof(wchar_t) == sizeof(QChar));
}

// Convert "\\Device\\HarddiskVolume1" to "C:"
QString FileUtil::dosNameToDrive(const QString &dosName)
{
    const QString dosNameLower = dosName.toLower();

    foreach (const QFileInfo &fi, QDir::drives()) {
        const QString driveName = fi.path().left(2);
        const QString driveDosName = driveToDosName(driveName);

        if (dosNameLower == driveDosName.toLower()) {
            return driveName;
        }
    }
    return QString();
}

// Convert "C:" to "\\Device\\HarddiskVolume1"
QString FileUtil::driveToDosName(const QString &drive)
{
    char driveName[3] = {drive.at(0).toLatin1(), ':', '\0'};

    char buf[MAX_PATH];
    const int len = QueryDosDeviceA((LPCSTR) driveName,
                                    buf, MAX_PATH);

    return (len > 0) ? QString::fromLatin1(buf)
                     : QString();
}

// Convert "\\Device\\HarddiskVolume1\\path" to "C:\\path"
QString FileUtil::dosPathToPath(const QString &dosPath)
{
    const QLatin1Char sep('\\');

    if (dosPath.startsWith(sep)) {
        const int sepPos1 = dosPath.indexOf(sep, 1);
        if (sepPos1 > 0) {
            const int sepPos2 = dosPath.indexOf(sep, sepPos1 + 1);
            if (sepPos2 > 0) {
                const QString dosName = dosPath.left(sepPos2);
                return dosNameToDrive(dosName) + dosPath.mid(sepPos2);
            }
        }
    }
    return dosPath;
}

// Convert "C:\\path" to "\\Device\\HarddiskVolume1\\path"
QString FileUtil::pathToDosPath(const QString &path)
{
    const QString drive = path.left(2);

    if (drive.at(0).isLetter() && drive.at(1) == QLatin1Char(':')) {
        return driveToDosName(drive) + path.mid(2);
    }
    return path;
}
