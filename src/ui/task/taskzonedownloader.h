#ifndef TASKZONEDOWNLOADER_H
#define TASKZONEDOWNLOADER_H

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

    QString url() const { return m_url; }
    void setUrl(const QString &v) { m_url = v; }

    QString formData() const { return m_formData; }
    void setFormData(const QString &v) { m_formData = v; }

    QString pattern() const { return m_pattern; }
    void setPattern(const QString &v) { m_pattern = v; }

    QString rangeText() const { return m_rangeText; }

    QStringList parseAddresses(const QByteArray &buffer) const;

protected:
    void setupDownloader() override;

    QString parseBuffer(const QByteArray &buffer) const;

protected slots:
    void downloadFinished(bool success) override;

private:
    bool m_sort = false;

    int m_emptyNetMask = 32;

    QString m_url;
    QString m_formData;

    QString m_pattern;

    QString m_rangeText;
};

#endif // TASKZONEDOWNLOADER_H
