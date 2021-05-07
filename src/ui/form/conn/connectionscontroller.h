#ifndef CONNECTIONSCONTROLLER_H
#define CONNECTIONSCONTROLLER_H

#include <QObject>

class ConfManager;
class ConnListModel;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class TranslationManager;

class ConnectionsController : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionsController(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    ConnListModel *connListModel() const;
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // CONNECTIONSCONTROLLER_H
