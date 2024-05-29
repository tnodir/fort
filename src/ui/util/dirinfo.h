#ifndef DIRINFO_H
#define DIRINFO_H

#include <QObject>

using DirHandle = void *;

#define DIR_INVALID_HANDLE DirHandle(-1)

class DirInfo final
{
public:
    explicit DirInfo();
    ~DirInfo();

    const QString &path() const { return m_path; }
    void setPath(const QString &path);

    bool open();
    void close();

    bool isOpen() const { return m_handle != DIR_INVALID_HANDLE; }

    bool checkIsValid();

private:
    DirHandle m_handle = DIR_INVALID_HANDLE;

    QString m_path;
};

#endif // DIRINFO_H
