#ifndef POLICYLISTBOX_H
#define POLICYLISTBOX_H

#include <QWidget>

#include <conf/rules/policylist.h>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class PolicyListModel;
class TableView;

class PolicyListBox : public QWidget
{
    Q_OBJECT

public:
    explicit PolicyListBox(PolicyListType type, QWidget *parent = nullptr);

    PolicyListModel *listModel() const { return m_listModel; }
    PolicyListType listType() const;

    QLabel *label() const { return m_label; }
    TableView *tableView() const { return m_tableView; }

signals:
    void addPolicy(PolicyListType listType);
    void removePolicy(PolicyListType listType);
    void editPolicy(PolicyListType listType);

public slots:
    void onRetranslateUi();

private:
    void setupUi();
    QLayout *setupHeader();
    void setupTableView();

private:
    PolicyListModel *m_listModel = nullptr;

    QLabel *m_label = nullptr;
    QToolButton *m_btAddPolicy = nullptr;
    QToolButton *m_btRemovePolicy = nullptr;
    QToolButton *m_btEditPolicy = nullptr;
    TableView *m_tableView = nullptr;
};

#endif // POLICYLISTBOX_H
