#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>

class Device : public QObject
{
    Q_OBJECT

public:
    explicit Device(QObject *parent = nullptr);
    ~Device() override;

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
    void *m_handle;
};

#endif // DEVICE_H
