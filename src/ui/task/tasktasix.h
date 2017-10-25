#ifndef TASKTASIX_H
#define TASKTASIX_H

#include "taskworker.h"

class HttpDownloader;

class TaskTasix : public TaskWorker
{
    Q_OBJECT

public:
    explicit TaskTasix(QObject *parent = nullptr);

    HttpDownloader *downloader() const { return m_downloader; }

    static QString parseTasixBufer(const QByteArray &buffer);

protected:
    virtual void startDownloader() const;

    virtual QString parseBufer(const QByteArray &buffer) const {
        return parseTasixBufer(buffer);
    }

signals:

public slots:
    void run() override;
    void cancel(bool success = false) override;

    bool processResult(FortManager *fortManager) override;

private slots:
    void downloadFinished(bool success);

private:
    HttpDownloader *m_downloader;

    QString m_rangeText;
};

#endif // TASKTASIX_H
