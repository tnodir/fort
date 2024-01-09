#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QVarLengthArray>

#include "classhelpers.h"

class Device : public QObject
{
    Q_OBJECT

public:
    enum OpenFlag : qint8 {
        ReadOnly = 0x01,
        ReadWrite = 0x02,
        Overlapped = 0x04,
    };
    Q_DECLARE_FLAGS(OpenFlags, OpenFlag)

    explicit Device(QObject *parent = nullptr);
    ~Device() override;
    CLASS_DELETE_COPY_MOVE(Device)

    bool isOverlapped() const { return (m_flags & Overlapped) != 0; }

    bool isOpened() const;

public slots:
    bool open(const QString &filePath, quint32 flags = ReadWrite);
    bool close();

    bool cancelIo();

    bool ioctl(quint32 code, char *in = nullptr, int inSize = 0, char *out = nullptr,
            int outSize = 0, qsizetype *retSize = nullptr);

    void initOverlapped(void *eventHandle = nullptr);

private:
    quint32 m_flags = 0;

    void *m_handle = nullptr;

    QVarLengthArray<char, 4 * sizeof(qintptr)> m_buffer;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Device::OpenFlags)

#endif // DEVICE_H
