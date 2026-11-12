#ifndef CONTROLCOMMANDMANAGER_H
#define CONTROLCOMMANDMANAGER_H

#include <control/control_types.h>

class ControlCommandManager
{
public:
    // Client: Prepare the arguments, post the command and process its result
    static bool processCommandClient(
            Control::Command command, QVariantList &args, ProcessCommandResult &r);

    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    // The command's result to the client, later by the command deferring it
    static void sendResult(ControlWorker *w, quint32 requestId, const ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDMANAGER_H
