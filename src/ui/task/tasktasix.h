#ifndef TASKTASIX_H
#define TASKTASIX_H

#include <QTimer>

#include "task.h"

class QNetworkAccessManager;
class QNetworkReply;

class TaskTasix : public Task
{
    Q_OBJECT

public:
    explicit TaskTasix(FortManager *fortManager,
                       QObject *parent = nullptr);

    static QString parseBufer(const QByteArray &buffer);

signals:
    void addressesReady(const QString &rangeText);

public slots:
    void run() override;
    void cancel() override;

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
