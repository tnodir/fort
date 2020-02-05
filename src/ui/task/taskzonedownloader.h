#ifndef TASKZONEDOWNLOADER_H
#define TASKZONEDOWNLOADER_H

#include <QDateTime>

#include "taskdownloader.h"

class TaskZoneDownloader : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskZoneDownloader(QObject *parent = nullptr);

    bool sort() const { return m_sort; }
    void setSort(bool v) { m_sort = v; }

    int emptyNetMask() const { return m_emptyNetMask; }
    void setEmptyNetMask(int v) { m_emptyNetMask = v; }

    qint64 zoneId() const { return m_zoneId; }
    void setZoneId(qint64 v) { m_zoneId = v; }

    QString url() const { return m_url; }
    void setUrl(const QString &v) { m_url = v; }

    QString formData() const { return m_formData; }
    void setFormData(const QString &v) { m_formData = v; }

    QString pattern() const { return m_pattern; }
    void setPattern(const QString &v) { m_pattern = v; }

    QString checksum() const { return m_checksum; }
    void setChecksum(const QString &v) { m_checksum = v; }

    QString cachePath() const { return m_cachePath; }
    void setCachePath(const QString &v) { m_cachePath = v; }

    QDateTime lastSuccess() const { return m_lastSuccess; }
    void setLastSuccess(const QDateTime &v) { m_lastSuccess = v; }

    QVector<QStringRef> parseAddresses(const QString &text,
                                       QString &checksum) const;
    bool saveAddresses(const QVector<QStringRef> &list,
                       const QString &filePath,
                       bool isOutputText = false) const;

    QString cacheFilePath() const;

protected:
    void setupDownloader() override;

protected slots:
    void downloadFinished(bool success) override;

private:
    bool m_sort = false;

    int m_emptyNetMask = 32;

    qint64 m_zoneId = 0;

    QString m_url;
    QString m_formData;

    QString m_pattern;

    QString m_checksum;
    QString m_cachePath;

    QDateTime m_lastSuccess;
};

#endif // TASKZONEDOWNLOADER_H
