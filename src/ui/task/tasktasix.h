#ifndef TASKTASIX_H
#define TASKTASIX_H

#include "taskdownloader.h"

class TaskTasix : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskTasix(QObject *parent = nullptr);

    static QStringList parseTasixBuffer(const QByteArray &buffer);

protected:
    void setupDownloader() override;

    QString parseBuffer(const QByteArray &buffer) const;

    virtual QStringList parseCustomBuffer(const QByteArray &buffer) const {
        return parseTasixBuffer(buffer);
    }

    virtual QString successMessage() const {
        return tr("TAS-IX addresses updated!");
    }

signals:

public slots:
    bool processResult(FortManager *fortManager) override;

protected slots:
    void downloadFinished(bool success) override;

private:
    QString m_rangeText;
};

#endif // TASKTASIX_H
