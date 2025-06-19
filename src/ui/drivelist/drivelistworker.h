#ifndef DRIVELISTWORKER_H
#define DRIVELISTWORKER_H

#include <QObject>
#include <QRunnable>

#include <util/classhelpers.h>
#include <util/device.h>

class DriveListWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit DriveListWorker(QObject *parent = nullptr);

    Device &device() { return m_device; }

    void run() override;

signals:
    void driveListChanged();

public slots:
    void close();

private:
    bool openDevice();
    void closeDevice();

    bool readEpicNumber();

private:
    volatile bool m_aborted = false;

    unsigned long m_epicNumber = 0; // MOUNTMGR_CHANGE_NOTIFY_INFO

    Device m_device;
};

#endif // DRIVELISTWORKER_H
