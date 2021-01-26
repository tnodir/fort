#ifndef QUOTAMANAGER_H
#define QUOTAMANAGER_H

#include <QObject>

class FortSettings;

class QuotaManager : public QObject
{
    Q_OBJECT

public:
    explicit QuotaManager(FortSettings *fortSettings, QObject *parent = nullptr);

    void setQuotaDayBytes(qint64 bytes);
    void setQuotaMonthBytes(qint64 bytes);

    void setTrafDayBytes(qint64 bytes);
    void setTrafMonthBytes(qint64 bytes);

    void clear(bool clearDay = true, bool clearMonth = true);
    void addTraf(qint64 bytes);

    void checkQuotaDay(qint32 trafDay);
    void checkQuotaMonth(qint32 trafMonth);

signals:
    void alert(const QString &text, const QString &title = tr("Quota Alert"));

public slots:

private:
    qint32 quotaDayAlerted() const;
    void setQuotaDayAlerted(qint32 v);

    qint32 quotaMonthAlerted() const;
    void setQuotaMonthAlerted(qint32 v);

private:
    qint32 m_quotaDayAlerted = 0;
    qint32 m_quotaMonthAlerted = 0;

    qint64 m_quotaDayBytes = 0;
    qint64 m_quotaMonthBytes = 0;

    qint64 m_trafDayBytes = 0;
    qint64 m_trafMonthBytes = 0;

    FortSettings *m_fortSettings = nullptr;
};

#endif // QUOTAMANAGER_H
