#ifndef QUOTAMANAGER_H
#define QUOTAMANAGER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(FortSettings)

class QuotaManager : public QObject
{
    Q_OBJECT

public:
    explicit QuotaManager(FortSettings *fortSettings,
                          QObject *parent = nullptr);

    void setQuotaDayBytes(qint64 bytes);
    void setQuotaMonthBytes(qint64 bytes);

    void setTrafDayBytes(qint64 bytes);
    void setTrafMonthBytes(qint64 bytes);

    void clear(bool clearDay = true, bool clearMonth = true);
    void addTraf(qint64 bytes);

    void checkQuotaDay(qint32 trafDay);
    void checkQuotaMonth(qint32 trafMonth);

signals:
    void alert(const QString &text,
               const QString &title = tr("Quota Alert"));

public slots:

private:
    qint32 quotaDayAlerted() const;
    void setQuotaDayAlerted(qint32 v);

    qint32 quotaMonthAlerted() const;
    void setQuotaMonthAlerted(qint32 v);

private:
    qint32 m_quotaDayAlerted;
    qint32 m_quotaMonthAlerted;

    qint64 m_quotaDayBytes;
    qint64 m_quotaMonthBytes;

    qint64 m_trafDayBytes;
    qint64 m_trafMonthBytes;

    FortSettings *m_fortSettings;
};

#endif // QUOTAMANAGER_H
