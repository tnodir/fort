#ifndef TASKZONEDOWNLOADER_H
#define TASKZONEDOWNLOADER_H

#include "taskdownloader.h"

class TaskZoneDownloader : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskZoneDownloader(QObject *parent = nullptr);

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

#endif // TASKZONEDOWNLOADER_H
