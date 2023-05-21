#include "fileutil.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimeZone>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace FileUtil {

Q_STATIC_ASSERT(sizeof(wchar_t) == sizeof(QChar));

QString systemAppDescription()
{
    return QStringLiteral("NT Kernel & System");
}

QString systemApp()
{
    return QStringLiteral("System");
}

bool isSystemApp(const QString &path)
{
    return QString::compare(path, systemApp(), Qt::CaseInsensitive) == 0;
}

QString svcHostPrefix()
{
    return QStringLiteral(R"(\svchost\)");
}

bool isSvcHostService(const QString &path, QString &serviceName)
{
    if (!path.startsWith(svcHostPrefix(), Qt::CaseInsensitive))
        return false;

    serviceName = path.mid(svcHostPrefix().size());

    return true;
}

bool isDriveFilePath(const QString &path)
{
    return path.size() > 1 && path[0].isLetter() && path[1] == ':';
}

// Convert "\\Device\\HarddiskVolume1" to "C:"
QString kernelNameToDrive(const QString &kernelName)
{
    const QString kernelNameLower = kernelName.toLower();

    const auto drives = QDir::drives();

    for (const QFileInfo &fi : drives) {
        const QString driveName = fi.path().left(2);
        const QString driveKernelName = driveToKernelName(driveName);

        if (kernelNameLower == driveKernelName.toLower()) {
            return driveName;
        }
    }
    return QString();
}

// Convert "C:" to "\\Device\\HarddiskVolume1"
QString driveToKernelName(const QString &drive)
{
    char driveName[3] = { drive.at(0).toLatin1(), ':', '\0' };

    char buf[MAX_PATH];
    const int len = QueryDosDeviceA((LPCSTR) driveName, buf, MAX_PATH);

    return (len > 0) ? QString::fromLatin1(buf) : QString();
}

// Convert "\\Device\\HarddiskVolume1\\path" to "C:\\path"
QString kernelPathToPath(const QString &kernelPath)
{
    const QLatin1Char sep('\\');

    if (kernelPath.startsWith(sep)) {
        const int sepPos1 = kernelPath.indexOf(sep, 1);
        if (sepPos1 > 0) {
            const int sepPos2 = kernelPath.indexOf(sep, sepPos1 + 1);
            if (sepPos2 > 0) {
                const QString kernelName = kernelPath.left(sepPos2);
                return kernelNameToDrive(kernelName) + kernelPath.mid(sepPos2);
            }
        }
    }
    return kernelPath;
}

// Convert "C:\\path" to "\\Device\\HarddiskVolume1\\path"
QString pathToKernelPath(const QString &path, bool lower)
{
    QString kernelPath = path;

    if (path.size() > 1) {
        const QChar char1 = path.at(0);
        if (char1.isLetter()) {
            if (path.at(1) == ':') {
                const QString drive = path.left(2);
                kernelPath = driveToKernelName(drive) + path.mid(2);
            } else if (isSystemApp(path)) {
                return systemApp();
            }
        } else if ((char1 == '?' || char1 == '*') && path.at(1) == ':') {
            // Replace "?:\\" with "\\Device\\*\\"
            kernelPath = "\\Device\\*" + path.mid(2);
        }
    }

    kernelPath = kernelPath.replace(QLatin1Char('/'), QLatin1Char('\\'));

    return lower ? kernelPath.toLower() : kernelPath;
}

QString fileName(const QString &path)
{
    return QFileInfo(path).fileName();
}

QString absolutePath(const QString &path)
{
    return QDir(path).absolutePath();
}

QString pathSlash(const QString &path)
{
    const QLatin1Char slash('/');
    return path.endsWith(slash) ? path : path + slash;
}

QString toNativeSeparators(const QString &path)
{
    return QDir::toNativeSeparators(path);
}

bool makePath(const QString &path)
{
    return QDir().mkpath(path);
}

bool makePathForFile(const QString &filePath)
{
    return QFileInfo(filePath).dir().mkpath(".");
}

bool pathExists(const QString &path)
{
    return QDir(path).exists();
}

bool fileExists(const QString &filePath)
{
    return QFileInfo::exists(filePath);
}

bool removeFile(const QString &filePath)
{
    return QFile::remove(filePath);
}

bool renameFile(const QString &oldFilePath, const QString &newFilePath)
{
    removeFile(newFilePath);

    return QFile::rename(oldFilePath, newFilePath);
}

bool copyFile(const QString &filePath, const QString &newFilePath)
{
    return QFile::copy(filePath, newFilePath);
}

bool linkFile(const QString &filePath, const QString &linkPath)
{
    return QFile::link(filePath, linkPath);
}

QString readFile(const QString &filePath)
{
    return QString::fromUtf8(readFileData(filePath));
}

QByteArray readFileData(const QString &filePath, qint64 maxSize)
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
        return QByteArray();

    return (maxSize <= 0) ? file.readAll() : file.read(maxSize);
}

bool writeFile(const QString &filePath, const QString &text)
{
    return writeFileData(filePath, text.toUtf8());
}

bool writeFileData(const QString &filePath, const QByteArray &data)
{
    makePathForFile(filePath); // create destination directory

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    return file.write(data) == data.size() && file.flush();
}

QDateTime fileModTime(const QString &filePath)
{
    const QFileInfo fi(filePath);
    return fi.lastModified(
#if QT_VERSION > QT_VERSION_CHECK(6, 5, 0)
            QTimeZone::UTC
#endif
    );
}

QString expandPath(const QString &path)
{
    constexpr int maxPathSize = 4096;
    wchar_t buf[maxPathSize];
    const int n = ExpandEnvironmentStringsW((LPCWSTR) path.utf16(), buf, maxPathSize);
    return (n > 0 && n < maxPathSize) ? QString::fromUtf16((const char16_t *) buf) : QString();
}

bool setCurrentDirectory(const QString &path)
{
    return QDir::setCurrent(path);
}

QString nativeAppFilePath()
{
    constexpr int maxPathSize = 4096;
    wchar_t buf[maxPathSize];
    const int n = GetModuleFileNameW(nullptr, buf, maxPathSize);
    return (n > 0 && n < maxPathSize) ? QString::fromUtf16((const char16_t *) buf) : QString();
}

QString nativeAppBinLocation()
{
    const QFileInfo fi(nativeAppFilePath());
    return fi.path();
}

QString appBinLocation()
{
    return QCoreApplication::applicationDirPath();
}

QString appConfigLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString applicationsLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
}

QString tempLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

void removeOldFiles(const QString &path, const QString &fileNamePrefix,
        const QString &fileNameSuffix, int keepFiles)
{
    QDir dir(path);
    const auto fileNames =
            dir.entryList({ fileNamePrefix + '*' + fileNameSuffix }, QDir::Files, QDir::Time);

    for (const QString &fileName : fileNames) {
        if (--keepFiles < 0) {
            dir.remove(fileName);
        }
    }
}

}
