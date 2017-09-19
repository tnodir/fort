#ifndef TASKTASIX_H
#define TASKTASIX_H

#include <QObject>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

class TaskTasix : public QObject
{
    Q_OBJECT

public:
    explicit TaskTasix(QObject *parent = nullptr);

    static QString parseBufer(const QByteArray &buffer);

signals:
    void finished();

public slots:
    void run();
    void cancel();

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
