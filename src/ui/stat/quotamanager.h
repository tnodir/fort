#ifndef QUOTAMANAGER_H
#define QUOTAMANAGER_H

#include <QObject>

class ConfManager;
class FirewallConf;
class IniOptions;

class QuotaManager : public QObject
{
    Q_OBJECT

public:
    enum AlertType : qint8 { AlertDay = 1, AlertMonth };

    explicit QuotaManager(ConfManager *confManager, QObject *parent = nullptr);

    void setQuotaDayBytes(qint64 bytes);
    void setQuotaMonthBytes(qint64 bytes);

    void setTrafDayBytes(qint64 bytes);
    void setTrafMonthBytes(qint64 bytes);

    void clear(bool clearDay = true, bool clearMonth = true);
    void addTraf(qint64 bytes);

    void checkQuotaDay(qint32 trafDay);
    void checkQuotaMonth(qint32 trafMonth);

    ConfManager *confManager() const { return m_confManager; }
    FirewallConf *conf() const;
    IniOptions *ini() const;

    static QString alertTypeText(qint8 alertType);

signals:
    void alert(qint8 alertType);

protected:
    virtual int quotaDayAlerted() const;
    virtual void setQuotaDayAlerted(qint32 v);

    virtual int quotaMonthAlerted() const;
    virtual void setQuotaMonthAlerted(qint32 v);

private:
    int m_quotaDayAlerted = 0;
    int m_quotaMonthAlerted = 0;

    qint64 m_quotaDayBytes = 0;
    qint64 m_quotaMonthBytes = 0;

    qint64 m_trafDayBytes = 0;
    qint64 m_trafMonthBytes = 0;

    ConfManager *m_confManager = nullptr;
};

#endif // QUOTAMANAGER_H
