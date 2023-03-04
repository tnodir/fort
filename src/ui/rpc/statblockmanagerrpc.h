#ifndef STATBLOCKMANAGERRPC_H
#define STATBLOCKMANAGERRPC_H

#include <stat/statblockmanager.h>

class StatBlockManagerRpc : public StatBlockManager
{
    Q_OBJECT

public:
    explicit StatBlockManagerRpc(const QString &filePath, QObject *parent = nullptr);

    void deleteConn(qint64 connIdTo = 0) override;

public slots:
    void onConnChanged();

protected:
    void setupWorker() override { }
    void setupConfManager() override { }
};

#endif // STATBLOCKMANAGERRPC_H
