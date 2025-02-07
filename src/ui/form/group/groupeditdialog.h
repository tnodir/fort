#ifndef GROUPEDITDIALOG_H
#define GROUPEDITDIALOG_H

#include <QDialog>

#include <model/grouplistmodel.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

class AppGroup;
class GroupListModel;
class GroupsController;
class LineEdit;
class PlainTextEdit;

class GroupEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GroupEditDialog(ZonesController *ctrl, QWidget *parent = nullptr);

    GroupsController *ctrl() const { return m_ctrl; }
    GroupListModel *groupListModel() const;

    bool isEmpty() const { return m_groupRow.groupId == 0; }

    void initialize(const GroupRow &groupRow);

protected slots:
    void retranslateUi();

private:
    void initializeFocus();

    void setupController();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupNameLayout();
    void setupTextFrame();
    QLayout *setupTextLayout();
    QLayout *setupButtons();

    bool save();
    bool saveGroup(AppGroup &group);

    void fillGroup(AppGroup &group) const;

private:
    GroupsController *m_ctrl = nullptr;

    QLabel *m_labelName = nullptr;
    LineEdit *m_editName = nullptr;
    QLabel *m_labelSource = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QFrame *m_frameUrl = nullptr;
    QCheckBox *m_cbCustomUrl = nullptr;
    QComboBox *m_comboSources = nullptr;
    QLabel *m_labelUrl = nullptr;
    LineEdit *m_editUrl = nullptr;
    QLabel *m_labelFormData = nullptr;
    LineEdit *m_editFormData = nullptr;
    QFrame *m_frameText = nullptr;
    PlainTextEdit *m_editText = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    GroupRow m_groupRow;
};

#endif // GROUPEDITDIALOG_H
