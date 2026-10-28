#ifndef CONTROLCOMMANDCONN_H
#define CONTROLCOMMANDCONN_H

#include <memory>

#include "controlcommandbase.h"

class AppConnSearchModel;
class ConnSearchModel;
struct ConnRow;

class ControlCommandConn : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

    // The program's connections' model, of all programs for the empty path
    template<class Model, class AppModel>
    static std::unique_ptr<ConnSearchModel> createConnModel(const QString &appPath)
    {
        if (appPath.isEmpty())
            return std::make_unique<Model>();

        auto model = std::make_unique<AppModel>();
        setModelApp(*model, appPath);

        return model;
    }

    // The connections' fields by lines
    static QString connsText(const QVector<ConnRow> &connRows);

private:
    static void setModelApp(AppConnSearchModel &model, const QString &appPath);
};

#endif // CONTROLCOMMANDCONN_H
