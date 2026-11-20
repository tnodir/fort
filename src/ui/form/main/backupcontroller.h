#ifndef BACKUPCONTROLLER_H
#define BACKUPCONTROLLER_H

#include <form/basecontroller.h>

class BackupController : public BaseController
{
    Q_OBJECT

public:
    explicit BackupController(QObject *parent = nullptr);

public slots:
    void exportBackup();
    void importBackup(bool onlyNewApps = false);
    void confirmImportBackup();
    void confirmImportAppsBackup();
};

#endif // BACKUPCONTROLLER_H
