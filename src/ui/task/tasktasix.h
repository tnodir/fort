#ifndef TASKTASIX_H
#define TASKTASIX_H

#include "taskworker.h"

class NetDownloader;

class TaskTasix : public TaskWorker
{
    Q_OBJECT

public:
    explicit TaskTasix(QObject *parent = nullptr);

    NetDownloader *downloader() const { return m_downloader; }

    static QStringList parseTasixBuffer(const QByteArray &buffer);

protected:
    virtual void setupDownloader() const;

    QString parseBuffer(const QByteArray &buffer) const;

    virtual QStringList parseCustomBuffer(const QByteArray &buffer) const {
        return parseTasixBuffer(buffer);
    }

    virtual QString successMessage() const {
        return tr("TAS-IX addresses updated!");
    }

signals:

public slots:
    void run() override;
    void abort(bool success = false) override;

    bool processResult(FortManager *fortManager) override;

private slots:
    void downloadFinished(bool success);

private:
    NetDownloader *m_downloader;

    QString m_rangeText;
};

#endif // TASKTASIX_H
