#ifndef TASKTASIX_H
#define TASKTASIX_H

#include <QTimer>

#include "taskworker.h"

class QNetworkAccessManager;
class QNetworkReply;

class TaskTasix : public TaskWorker
{
    Q_OBJECT

public:
    explicit TaskTasix(QObject *parent = nullptr);

    static QString parseBufer(const QByteArray &buffer);

signals:

public slots:
    void run() override;
    void cancel(bool success = false) override;

    bool processResult(FortManager *fortManager) override;

private slots:
    void requestReadyRead();
    void requestFinished();

private:
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_reply;

    QTimer m_timer;

    QByteArray m_buffer;
};

#endif // TASKTASIX_H
