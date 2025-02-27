#ifndef GROUPEDITDIALOG_H
#define GROUPEDITDIALOG_H

#include <QDialog>

#include <conf/group.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

class Group;
class GroupListModel;
class GroupsController;
class LineEdit;
class PlainTextEdit;

class GroupEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GroupEditDialog(GroupsController *ctrl, QWidget *parent = nullptr);

    GroupsController *ctrl() const { return m_ctrl; }
    GroupListModel *groupListModel() const;

    bool isEmpty() const { return m_group.groupId == 0; }

    void initialize(const Group &group);

protected slots:
    void retranslateUi();

private:
    void initializeFocus();

    void setupController();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupNameLayout();
    QLayout *setupNotesLayout();
    QLayout *setupButtons();

    bool save();
    bool saveGroup(Group &group);

    void fillGroup(Group &group) const;

private:
    GroupsController *m_ctrl = nullptr;

    QLabel *m_labelName = nullptr;
    LineEdit *m_editName = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QCheckBox *m_cbExclusive = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    Group m_group;
};

#endif // GROUPEDITDIALOG_H
