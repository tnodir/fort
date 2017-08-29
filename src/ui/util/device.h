#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>

class Device : public QObject
{
    Q_OBJECT

public:
    explicit Device(QObject *parent = nullptr);
    virtual ~Device();

signals:

public slots:
    bool open(const QString &filePath);
    bool close();

    bool ioctl(int code, QByteArray *in = nullptr, QByteArray *out = nullptr,
               int *retSize = nullptr);

private:
    void *m_handle;
};

#endif // DEVICE_H
