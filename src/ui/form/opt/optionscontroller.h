#ifndef OPTIONSCONTROLLER_H
#define OPTIONSCONTROLLER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(DriverManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(TaskManager)
QT_FORWARD_DECLARE_CLASS(TranslationManager)
QT_FORWARD_DECLARE_CLASS(ZoneListModel)

class OptionsController : public QObject
{
    Q_OBJECT

public:
    explicit OptionsController(FortManager *fortManager, QObject *parent = nullptr);

    bool confFlagsEdited() const { return m_confFlagsEdited; }
    void setConfFlagsEdited(bool v);

    bool confEdited() const { return m_confEdited; }
    void setConfEdited(bool v);

    bool othersEdited() const { return m_othersEdited; }
    void setOthersEdited(bool v);

    bool anyEdited() const { return confFlagsEdited() || confEdited() || othersEdited(); }

    void resetEdited();

    void initialize();

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    TaskManager *taskManager() const;
    DriverManager *driverManager() const;
    TranslationManager *translationManager() const;
    ZoneListModel *zoneListModel() const;

signals:
    void editedChanged();
    void editResetted();

    void aboutToSave();
    void saved();

    void retranslateUi();

public slots:
    void closeWindow();

    void saveChanges() { save(true); }
    void applyChanges() { save(false); }

private:
    void save(bool closeOnSuccess);

private:
    bool m_confFlagsEdited : 1;
    bool m_confEdited : 1;
    bool m_othersEdited : 1;

    FortManager *m_fortManager = nullptr;
};

#endif // OPTIONSCONTROLLER_H
