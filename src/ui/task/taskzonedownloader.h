#ifndef TASKZONEDOWNLOADER_H
#define TASKZONEDOWNLOADER_H

#include "taskdownloader.h"

class TaskZoneDownloader : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskZoneDownloader(QObject *parent = nullptr);

    QString rangeText() const { return m_rangeText; }

    static QStringList parseAddresses(const QByteArray &buffer);

protected:
    void setupDownloader() override;

    QString parseBuffer(const QByteArray &buffer) const;

protected slots:
    void downloadFinished(bool success) override;

private:
    QString m_rangeText;
};

#endif // TASKZONEDOWNLOADER_H
