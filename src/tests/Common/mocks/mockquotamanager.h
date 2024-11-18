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
    MOCK_METHOD(qint32, quotaDayAlerted, (), (const));
    MOCK_METHOD(void, setQuotaDayAlerted, (qint32 v));

    MOCK_METHOD(qint32, quotaMonthAlerted, (), (const));
    MOCK_METHOD(void, setQuotaMonthAlerted, (qint32 v));
};

#endif // MOCKQUOTAMANAGER_H
