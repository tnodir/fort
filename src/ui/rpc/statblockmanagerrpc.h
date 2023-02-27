#ifndef STATBLOCKMANAGERRPC_H
#define STATBLOCKMANAGERRPC_H

#include <stat/statblockmanager.h>

class StatBlockManagerRpc : public StatBlockManager
{
    Q_OBJECT

public:
    explicit StatBlockManagerRpc(const QString &filePath, QObject *parent = nullptr);

    bool deleteConn(qint64 rowIdTo) override;
    bool deleteConnAll() override;

public slots:
    void onConnChanged();
};

#endif // STATBLOCKMANAGERRPC_H
