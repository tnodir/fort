#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <QDateTime>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QDir)

namespace FileUtil {

QString systemAppDescription();
QString systemApp();
bool isSystemApp(const QString &path);

QString svcHostPrefix();
bool isSvcHostService(const QString &path, QString &serviceName);

bool isDriveFilePath(const QString &path);

quint32 driveMask();

quint32 mountedDriveMask(quint32 driveMask);

quint32 driveMaskByPath(const QString &path);

// Convert DOS device name to drive letter (A: .. Z:)
QString kernelNameToDrive(const QString &kernelName);

// Convert drive letter (A: .. Z:) to DOS device name
QString driveToKernelName(const QString &drive);

// Convert Native kernel path to Win32 path
QString kernelPathToPath(const QString &kernelPath);

// Convert Win32 path to Native kernel path
QString pathToKernelPath(const QString &path, bool lower = true);

QString fileName(const QString &path);

QString absolutePath(const QString &path);
QString pathSlash(const QString &path);
QString toNativeSeparators(const QString &path);

// Normalize the Win32 path
QString normalizePath(const QString &path);

bool removePath(const QString &path);
bool makePath(const QString &path);
bool makePathForFile(const QString &filePath);
bool pathExists(const QString &path);
bool fileExists(const QString &filePath);
bool removeFile(const QString &filePath);
bool renameFile(const QString &oldFilePath, const QString &newFilePath);
bool copyFile(const QString &filePath, const QString &newFilePath);
bool replaceFile(const QString &filePath, const QString &newFilePath);
bool linkFile(const QString &filePath, const QString &linkPath);

QString readFile(const QString &filePath);
QByteArray readFileData(const QString &filePath, qint64 maxSize = -1);

bool writeFile(const QString &filePath, const QString &text);
bool writeFileData(const QString &filePath, const QByteArray &data);

QDateTime fileModTime(const QString &filePath);

QString expandPath(const QString &path);

bool setCurrentDirectory(const QString &path);

QString nativeAppFilePath();
QString nativeAppBinLocation();
QString appBinLocation();
QString appConfigLocation();
QString applicationsLocation();
QString tempLocation();

QStringList getFileNames(QDir &dir, const QString &fileNamePrefix, const QString &fileNameSuffix);
void removeOldFiles(
        QDir &dir, const QString &fileNamePrefix, const QString &fileNameSuffix, int keepFiles);
};

#endif // FILEUTIL_H
