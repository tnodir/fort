#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <QObject>

class FileUtil : public QObject
{
    Q_OBJECT

public:
    explicit FileUtil(QObject *parent = nullptr);

    // Convert DOS device name to drive letter (A: .. Z:)
    static QString dosNameToDrive(const QString &dosName);

    // Convert drive letter (A: .. Z:) to DOS device name
    static QString driveToDosName(const QString &drive);

    // Convert Native path to Win32 path
    static QString dosPathToPath(const QString &dosPath);

    // Convert Win32 path to Native path
    static QString pathToDosPath(const QString &path);

    static QString absolutePath(const QString &path);

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

    static QString appConfigLocation();
    static QString applicationsLocation();
};

#endif // FILEUTIL_H
