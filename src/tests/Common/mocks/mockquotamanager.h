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
    MOCK_CONST_METHOD0(quotaDayAlerted, qint32());
    MOCK_METHOD1(setQuotaDayAlerted, void(qint32 v));

    MOCK_CONST_METHOD0(quotaMonthAlerted, qint32());
    MOCK_METHOD1(setQuotaMonthAlerted, void(qint32 v));
};

#endif // MOCKQUOTAMANAGER_H
