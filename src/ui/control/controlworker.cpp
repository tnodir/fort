#include "controlworker.h"

#include <QDataStream>
#include <QSharedMemory>
#include <QSystemSemaphore>

ControlWorker::ControlWorker(QSystemSemaphore *semaphore,
                             QSharedMemory *sharedMemory,
                             QObject *parent) :
    QObject(parent),
    m_semaphore(semaphore),
    m_sharedMemory(sharedMemory)
{
}

void ControlWorker::run()
{
    QMutexLocker locker(&m_mutex);

    while (!m_aborted && m_semaphore->acquire()) {
        processRequest();
    }
}

void ControlWorker::abort()
{
    m_aborted = true;

    m_semaphore->release();

    // Wait thread finishing
    {
        QMutexLocker locker(&m_mutex);

        m_semaphore = nullptr;
        m_sharedMemory = nullptr;
    }
}

bool ControlWorker::post(const QString &command,
                         const QStringList &args)
{
    m_sharedMemory->lock();

    const bool res = writeDataStream(command, args);

    m_sharedMemory->unlock();

    if (res) {
        m_semaphore->release();
    }

    return res;
}

void ControlWorker::processRequest()
{
    QString scriptPath;
    QStringList args;

    m_sharedMemory->lock();

    const bool res = readDataStream(scriptPath, args);

    m_sharedMemory->unlock();

    if (res) {
        emit requestReady(scriptPath, args);
    }
}

bool ControlWorker::writeData(const QByteArray &data)
{
    const int dataSize = data.size();

    if (dataSize < 0 || int(sizeof(int)) + dataSize > m_sharedMemory->size())
        return false;

    int *p = static_cast<int *>(m_sharedMemory->data());
    *p++ = dataSize;

    if (dataSize != 0) {
        memcpy(p, data.constData(), size_t(dataSize));
    }

    return true;
}

QByteArray ControlWorker::readData() const
{
    const int *p = static_cast<const int *>(m_sharedMemory->constData());
    const int dataSize = *p++;

    if (dataSize < 0 || int(sizeof(int)) + dataSize > m_sharedMemory->size())
        return QByteArray();

    return QByteArray::fromRawData(reinterpret_cast<const char *>(p), dataSize);
}

bool ControlWorker::writeDataStream(const QString &scriptPath,
                                    const QStringList &args)
{
    QByteArray data;

    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << scriptPath << args;

    return writeData(data);
}

bool ControlWorker::readDataStream(QString &scriptPath,
                                   QStringList &args) const
{
    const QByteArray data = readData();

    if (data.isEmpty())
        return false;

    QDataStream stream(data);
    stream >> scriptPath >> args;

    return true;
}
