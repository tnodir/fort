#ifndef CONTROLCOMMANDBASE_H
#define CONTROLCOMMANDBASE_H

#include <control/control_types.h>

class ControlCommandBase
{
public:
    enum ClientFileArg : qint8 {
        ClientFileNone = 0,
        ClientFileRead,
        ClientFileWrite,
    };

    // Client: Read or write the files of the user
    bool prepareClientArgs(QVariantList &args, ProcessCommandResult &r) const;
    bool processClientResult(const QVariantList &args, ProcessCommandResult &r) const;

    virtual bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const = 0;

    // The command shows the windows, which the service has none of
    virtual bool needsWindows(const QVariantList & /*args*/) const { return false; }

protected:
    // Client: The <file-path> argument's file to send its text instead of the path,
    // or to write the result's text to
    virtual ClientFileArg clientFileArg(const QVariantList & /*args*/) const
    {
        return ClientFileNone;
    }
};

#endif // CONTROLCOMMANDBASE_H
