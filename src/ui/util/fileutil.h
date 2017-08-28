#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <QObject>
#include <QHash>

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
};

#endif // FILEUTIL_H
