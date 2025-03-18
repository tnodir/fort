#ifndef DRIVELISTMANAGERRPC_H
#define DRIVELISTMANAGERRPC_H

#include <manager/drivelistmanager.h>

struct ProcessCommandArgs;

class DriveListManagerRpc : public DriveListManager
{
    Q_OBJECT

public:
    explicit DriveListManagerRpc(QObject *parent = nullptr);

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

public slots:
    void onDriveListChanged() override;

protected slots:
    void populateDriveMask() override { }
};

#endif // DRIVELISTMANAGERRPC_H
