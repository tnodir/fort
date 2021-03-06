#ifndef ZONESCONTROLLER_H
#define ZONESCONTROLLER_H

#include <QObject>

class ZoneListModel;
class ConfManager;
class FortManager;
class FortSettings;
class TranslationManager;

class ZonesController : public QObject
{
    Q_OBJECT

public:
    explicit ZonesController(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    ZoneListModel *zoneListModel() const;
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // ZONESCONTROLLER_H
