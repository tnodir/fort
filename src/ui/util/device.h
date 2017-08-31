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

    bool ioctl(int code, QByteArray *in = nullptr, int inSize = 0,
               QByteArray *out = nullptr, int outSize = 0,
               int *retSize = nullptr);

private:
    void *m_handle;
};

#endif // DEVICE_H
