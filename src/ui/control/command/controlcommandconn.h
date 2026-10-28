#ifndef CONTROLCOMMANDCONN_H
#define CONTROLCOMMANDCONN_H

#include <memory>

#include "controlcommandbase.h"

class AppConnSearchModel;
class ConnSearchModel;
class ControlCommandConn : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

private:
    static void setModelApp(AppConnSearchModel &model, const QString &appPath);
};

#endif // CONTROLCOMMANDCONN_H
