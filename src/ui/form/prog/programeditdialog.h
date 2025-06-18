#ifndef PROGRAMEDITDIALOG_H
#define PROGRAMEDITDIALOG_H

#include "programeditdialogbase.h"

class ProgramEditDialog : public ProgramEditDialogBase
{
    Q_OBJECT

public:
    explicit ProgramEditDialog(
            ProgramEditController *ctrl, QWidget *parent = nullptr, Qt::WindowFlags f = {});

    void initialize(const App &app, const QVector<qint64> &appIdList = {});

protected:
    void saveScheduleAction(App::ScheduleAction actionType, int minutes) override;

    bool save() override;

private:
    void initializePathNameRuleFields(bool isSingleSelection = true);
    void initializePathField(bool isSingleSelection);
    void initializeNameField(bool isSingleSelection);
    void initializeNotesField(bool isSingleSelection);
    void initializeRuleField(bool isSingleSelection);
    void initializeFocus();

    void setupUi();

    bool saveApp(App &app);
    bool saveMulti(App &app);

    bool validateFields() const;
    void fillApp(App &app) const;
    void fillAppPath(App &app) const;
    void fillAppApplyChild(App &app) const;
    void fillAppEndTime(App &app) const;
};

#endif // PROGRAMEDITDIALOG_H
