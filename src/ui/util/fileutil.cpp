#include "fileutil.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimeZone>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>
#include <winioctl.h>

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

quint32 driveMask()
{
    return GetLogicalDrives();
}

static bool isDriveMounted(WCHAR drive)
{
    const WCHAR volume[] = { L'\\', L'\\', L'.', L'\\', drive, L':', L'\0' };

    const DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const HANDLE volumeHandle =
            CreateFileW(volume, GENERIC_READ, shareMode, nullptr, OPEN_EXISTING, 0, nullptr);

    if (volumeHandle == INVALID_HANDLE_VALUE)
        return false;

    DWORD nr;
    const bool ok = DeviceIoControl(
            volumeHandle, FSCTL_IS_VOLUME_MOUNTED, nullptr, 0, nullptr, 0, &nr, nullptr);

    CloseHandle(volumeHandle);

    return ok;
}

quint32 mountedDriveMask(quint32 driveMask)
{
    quint32 mask = driveMask;
    while (mask != 0) {
        unsigned long index;
        if (!_BitScanForward(&index, mask))
            break;

        const quint32 bit = (1u << index);

        if (!isDriveMounted(L'A' + index)) {
            driveMask ^= bit;
        }

        mask ^= bit;
    }

    return driveMask;
}

quint32 driveMaskByPath(const QString &path)
{
    if (!isDriveFilePath(path))
        return 0;

    const char16_t c = path.at(0).toUpper().unicode();
    if (Q_UNLIKELY(c < 'A' || c > 'Z'))
        return 0;

    return 1U << (c - 'A');
}

// Convert "\\Device\\HarddiskVolume1" to "C:"
QString kernelNameToDrive(const QString &kernelName)
{
    if (kernelName.isEmpty())
        return QString();

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

inline QString getKernelName(const QString &kernelPath)
{
    const QLatin1Char sep('\\');

    if (kernelPath.startsWith(sep)) {
        const int sepPos1 = kernelPath.indexOf(sep, 1);
        if (sepPos1 > 0) {
            const int sepPos2 = kernelPath.indexOf(sep, sepPos1 + 1);
            if (sepPos2 > 0) {
                return kernelPath.left(sepPos2);
            }
        }
    }

    return QString();
}

// Convert "\\Device\\HarddiskVolume1\\path" to "C:\\path"
QString kernelPathToPath(const QString &kernelPath)
{
    const QString kernelName = getKernelName(kernelPath);
    const QString driveName = kernelNameToDrive(kernelName);

    if (!driveName.isEmpty()) {
        return driveName + kernelPath.mid(kernelName.size());
    }

    return kernelPath;
}

inline QString convertDrivePathToKernelPath(const QString &path)
{
    if (path.at(1) == ':') {
        const QString drive = path.left(2);
        return driveToKernelName(drive) + path.mid(2);
    }

    return path;
}

inline bool isWildDrive(const QString &path)
{
    const QChar char1 = path.at(0);
    return ((char1 == '?' || char1 == '*') && path.at(1) == ':');
}

inline QString convertWildPathToKernelPath(const QString &path)
{
    if (isWildDrive(path)) {
        // Replace "?:\\" with "\\Device\\*\\"
        return "\\Device\\*" + path.mid(2);
    }

    return path;
}

inline QString convertPathToKernelPath(const QString &path)
{
    if (path.size() <= 1)
        return path;

    const QChar char1 = path.at(0);
    const QString kernelPath = char1.isLetter() ? convertDrivePathToKernelPath(path)
                                                : convertWildPathToKernelPath(path);

    return toNativeSeparators(kernelPath);
}

// Convert "C:\\path" to "\\Device\\HarddiskVolume1\\path"
QString pathToKernelPath(const QString &path, bool lower)
{
    if (isSystemApp(path))
        return systemApp();

    const QString kernelPath = convertPathToKernelPath(path);

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

QString normalizePath(const QString &path)
{
    if (path.isEmpty())
        return {};

    const QString pathTrimmed = path.trimmed();
    if (pathTrimmed.isEmpty())
        return {};

    if (isSystemApp(pathTrimmed))
        return systemApp();

    QString pathLower = pathTrimmed.toLower();

    if (isDriveFilePath(pathLower)) {
        pathLower[0] = pathLower[0].toUpper();
    }

    return toNativeSeparators(pathLower);
}

bool removePath(const QString &path)
{
    return QDir(path).removeRecursively();
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

bool replaceFile(const QString &filePath, const QString &newFilePath)
{
    FileUtil::removeFile(newFilePath);

    return FileUtil::copyFile(filePath, newFilePath);
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
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

QStringList getFileNames(QDir &dir, const QString &fileNamePrefix, const QString &fileNameSuffix)
{
    return dir.entryList({ fileNamePrefix + '*' + fileNameSuffix }, QDir::Files, QDir::Time);
}

void removeOldFiles(
        QDir &dir, const QString &fileNamePrefix, const QString &fileNameSuffix, int keepFiles)
{
    const auto fileNames = getFileNames(dir, fileNamePrefix, fileNameSuffix);

    for (const QString &fileName : fileNames) {
        if (--keepFiles <= 0) {
            dir.remove(fileName);
        }
    }
}

}
