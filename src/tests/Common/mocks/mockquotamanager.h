#ifndef MOCKQUOTAMANAGER_H
#define MOCKQUOTAMANAGER_H

#include <googletest.h>

#include <stat/quotamanager.h>

class MockQuotaManager : public QuotaManager
{
    Q_OBJECT

public:
    explicit MockQuotaManager(QObject *parent = nullptr);

protected:
    MOCK_METHOD(qint32, quotaAlerted, (QuotaManager::AlertType alertType), (const));
    MOCK_METHOD(void, setQuotaAlerted, (QuotaManager::AlertType alertType, qint32 v));
};

#endif // MOCKQUOTAMANAGER_H
