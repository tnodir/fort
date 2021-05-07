#ifndef ZONESCONTROLLER_H
#define ZONESCONTROLLER_H

#include <QObject>

class ConfManager;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class TranslationManager;
class ZoneListModel;

class ZonesController : public QObject
{
    Q_OBJECT

public:
    explicit ZonesController(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    ZoneListModel *zoneListModel() const;
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // ZONESCONTROLLER_H
