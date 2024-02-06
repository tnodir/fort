#ifndef RULELISTBOX_H
#define RULELISTBOX_H

#include <QWidget>

#include <conf/rules/policy.h>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class RuleListModel;
class TableView;

class RuleListBox : public QWidget
{
    Q_OBJECT

public:
    explicit RuleListBox(Policy::PolicyType policyType, QWidget *parent = nullptr);

    RuleListModel *listModel() const { return m_listModel; }
    Policy::PolicyType policyType() const;

    QLabel *label() const { return m_label; }
    TableView *tableView() const { return m_tableView; }

signals:
    void addPolicy(Policy::PolicyType type);
    void removePolicy(Policy::PolicyType type);
    void editPolicy(Policy::PolicyType type);

public slots:
    void onRetranslateUi();

private:
    void setupUi();
    QLayout *setupHeader();
    void setupTableView();

private:
    RuleListModel *m_listModel = nullptr;

    QLabel *m_label = nullptr;
    QToolButton *m_btAddPolicy = nullptr;
    QToolButton *m_btRemovePolicy = nullptr;
    QToolButton *m_btEditPolicy = nullptr;
    TableView *m_tableView = nullptr;
};

#endif // RULELISTBOX_H
