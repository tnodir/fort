#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <QDateTime>
#include <QObject>

class FileUtil
{
public:
    static QString systemApp();
    static bool isSystemApp(const QString &path);

    // Convert DOS device name to drive letter (A: .. Z:)
    static QString kernelNameToDrive(const QString &kernelName);

    // Convert drive letter (A: .. Z:) to DOS device name
    static QString driveToKernelName(const QString &drive);

    // Convert Native path to Win32 path
    static QString kernelPathToPath(const QString &kernelPath);

    // Convert Win32 path to Native path
    static QString pathToKernelPath(const QString &path, bool lower = true);

    static QString fileName(const QString &path);

    static QString absolutePath(const QString &path);
    static QString pathSlash(const QString &path);
    static QString toNativeSeparators(const QString &path);

    static bool makePath(const QString &path);
    static bool fileExists(const QString &filePath);
    static bool removeFile(const QString &filePath);
    static bool renameFile(const QString &oldFilePath, const QString &newFilePath);
    static bool copyFile(const QString &filePath, const QString &newFilePath);
    static bool linkFile(const QString &filePath, const QString &linkPath);

    static QString readFile(const QString &filePath);
    static QByteArray readFileData(const QString &filePath);

    static bool writeFile(const QString &filePath, const QString &text);
    static bool writeFileData(const QString &filePath, const QByteArray &data);

    static QDateTime fileModTime(const QString &filePath);

    static QString appBinLocation();
    static QString appCacheLocation();
    static QString appConfigLocation();
    static QString applicationsLocation();
};

#endif // FILEUTIL_H
