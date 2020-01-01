#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>

#include "classhelpers.h"

class Device : public QObject
{
    Q_OBJECT

public:
    explicit Device(QObject *parent = nullptr);
    ~Device() override;
    CLASS_DELETE_COPY_MOVE(Device)

    bool isOpened() const;

signals:

public slots:
    bool open(const QString &filePath);
    bool close();

    bool cancelIo();

    bool ioctl(quint32 code, char *in = nullptr, int inSize = 0,
               char *out = nullptr, int outSize = 0,
               int *retSize = nullptr);

private:
    void *m_handle = nullptr;
};

#endif // DEVICE_H
