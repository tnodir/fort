#ifndef TASKTASIX_H
#define TASKTASIX_H

#include "taskdownloader.h"

class TaskTasix : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskTasix(QObject *parent = nullptr);

    QString rangeText() const { return m_rangeText; }

    static QStringList parseTasixBuffer(const QByteArray &buffer);

protected:
    void setupDownloader() override;

    QString parseBuffer(const QByteArray &buffer) const;

    virtual QStringList parseCustomBuffer(const QByteArray &buffer) const {
        return parseTasixBuffer(buffer);
    }

protected slots:
    void downloadFinished(bool success) override;

private:
    QString m_rangeText;
};

#endif // TASKTASIX_H
