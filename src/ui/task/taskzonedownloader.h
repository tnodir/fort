#ifndef TASKZONEDOWNLOADER_H
#define TASKZONEDOWNLOADER_H

#include <QDateTime>

#include <util/util_types.h>

#include "taskdownloader.h"

class TaskZoneDownloader : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskZoneDownloader(QObject *parent = nullptr);

    bool zoneEnabled() const { return m_zoneEnabled; }
    void setZoneEnabled(bool v) { m_zoneEnabled = v; }

    bool sort() const { return m_sort; }
    void setSort(bool v) { m_sort = v; }

    int emptyNetMask() const { return m_emptyNetMask; }
    void setEmptyNetMask(int v) { m_emptyNetMask = v; }

    int zoneId() const { return m_zoneId; }
    void setZoneId(int v) { m_zoneId = v; }

    int addressCount() const { return m_addressCount; }
    void setAddressCount(int v) { m_addressCount = v; }

    QString zoneName() const { return m_zoneName; }
    void setZoneName(const QString &v) { m_zoneName = v; }

    QString url() const { return m_url; }
    void setUrl(const QString &v) { m_url = v; }

    QString formData() const { return m_formData; }
    void setFormData(const QString &v) { m_formData = v; }

    QString textInline() const { return m_textInline; }
    void setTextInline(const QString &v) { m_textInline = v; }

    QString pattern() const { return m_pattern; }
    void setPattern(const QString &v) { m_pattern = v; }

    QString textChecksum() const { return m_textChecksum; }
    void setTextChecksum(const QString &v) { m_textChecksum = v; }

    QString binChecksum() const { return m_binChecksum; }
    void setBinChecksum(const QString &v) { m_binChecksum = v; }

    QString cachePath() const { return m_cachePath; }
    void setCachePath(const QString &v) { m_cachePath = v; }

    QDateTime sourceModTime() const { return m_sourceModTime; }
    void setSourceModTime(const QDateTime &v) { m_sourceModTime = v; }

    QDateTime lastSuccess() const { return m_lastSuccess; }
    void setLastSuccess(const QDateTime &v) { m_lastSuccess = v; }

    const QByteArray &zoneData() const { return m_zoneData; }

    StringViewList parseAddresses(const QString &text, QString &textChecksum) const;

    bool storeAddresses(const StringViewList &list);
    bool loadAddresses();

    bool saveAddressesAsText(const QString &filePath);

    QString cacheFileBasePath() const;
    QString cacheFileBinPath() const;

protected:
    void setupDownloader() override;

protected slots:
    void downloadFinished(const QByteArray &data, bool success) override;

private:
    void loadTextInline();
    void loadLocalFile();

private:
    bool m_zoneEnabled : 1 = false;
    bool m_sort : 1 = false;

    int m_emptyNetMask = 32;

    int m_zoneId = 0;

    int m_addressCount = 0;

    QString m_zoneName;

    QString m_url;
    QString m_formData;

    QString m_textInline;

    QString m_pattern;

    QString m_textChecksum;
    QString m_binChecksum;

    QString m_cachePath;

    QDateTime m_sourceModTime;
    QDateTime m_lastSuccess;

    QByteArray m_zoneData;
};

#endif // TASKZONEDOWNLOADER_H
