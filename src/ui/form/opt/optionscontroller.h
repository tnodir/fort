#ifndef OPTIONSCONTROLLER_H
#define OPTIONSCONTROLLER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(TaskManager)

class OptionsController : public QObject
{
    Q_OBJECT

public:
    explicit OptionsController(FortManager *fortManager,
                               QObject *parent = nullptr);

    bool confFlagsEdited() const { return m_confFlagsEdited; }
    void setConfFlagsEdited(bool v);

    bool confEdited() const { return m_confEdited; }
    void setConfEdited(bool v);

    bool othersEdited() const { return m_othersEdited; }
    void setOthersEdited(bool v);

    bool anyEdited() const {
        return confFlagsEdited() || confEdited() || othersEdited();
    }

    void resetEdited();

    void initialize();

    FortSettings *settings();
    FirewallConf *conf();
    TaskManager *taskManager();

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
    uint m_confFlagsEdited  : 1;
    uint m_confEdited       : 1;
    uint m_othersEdited     : 1;

    FortManager *m_fortManager = nullptr;
};

#endif // OPTIONSCONTROLLER_H
