#ifndef DRIVELISTMANAGERRPC_H
#define DRIVELISTMANAGERRPC_H

#include <control/control_types.h>
#include <manager/drivelistmanager.h>

class DriveListManagerRpc : public DriveListManager
{
    Q_OBJECT

public:
    explicit DriveListManagerRpc(QObject *parent = nullptr);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

public slots:
    void onDriveListChanged() override;

protected slots:
    void populateDriveMask() override { }
};

#endif // DRIVELISTMANAGERRPC_H
