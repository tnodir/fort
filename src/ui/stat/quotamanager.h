#ifndef QUOTAMANAGER_H
#define QUOTAMANAGER_H

#include <QObject>

#include <util/ioc/iocservice.h>

class IniOptions;

struct QuotaInfo
{
    qint32 quotaAlerted = 0;
    qint64 quotaBytes = 0;
    qint64 trafBytes = 0;
};

class QuotaManager : public QObject, public IocService
{
    Q_OBJECT

public:
    enum AlertType : qint8 { AlertDay = 0, AlertMonth, AlertCount };

    explicit QuotaManager(QObject *parent = nullptr);

    void setQuotaMBytes(AlertType alertType, qint64 mBytes);

    void setTrafBytes(AlertType alertType, qint64 bytes);

    void setUp() override;

    void clear();
    void clear(AlertType alertType);

    void addTraf(qint64 bytes);
    void addQuotaTraf(AlertType alertType, qint64 bytes);

    void checkQuota(AlertType alertType, qint32 trafAt);

    static QString alertTypeText(qint8 alertType);

signals:
    void alert(qint8 alertType);

protected:
    virtual void setupConfManager();

    virtual qint32 quotaAlerted(AlertType alertType) const;
    virtual void setQuotaAlerted(AlertType alertType, qint32 v);

    static int quotaAlertedByIni(AlertType alertType, IniOptions &ini);
    static void setQuotaAlertedByIni(AlertType alertType, qint32 v, IniOptions &ini);
    static QString quotaAlertedIniKey(AlertType alertType);

    constexpr QuotaInfo &quotaInfo(AlertType alertType) { return m_quotaInfos[alertType]; }

private:
    void processQuotaExceed(AlertType alertType);

    void setupByConfIni();

private:
    QuotaInfo m_quotaInfos[AlertCount];
};

#endif // QUOTAMANAGER_H
