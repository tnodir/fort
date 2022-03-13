#include "policylistbox.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <model/policylistmodel.h>

PolicyListBox::PolicyListBox(PolicyListType type, QWidget *parent) :
    QWidget(parent), m_listModel(new PolicyListModel(type, this))
{
    setupUi();

    listModel()->initialize();
}

PolicyListType PolicyListBox::listType() const
{
    return listModel()->type();
}

void PolicyListBox::onRetranslateUi()
{
    m_btAddPolicy->setToolTip(tr("Add Policy"));
    m_btRemovePolicy->setToolTip(tr("Remove Policy"));
    m_btEditPolicy->setToolTip(tr("Edit Policy"));
}

void PolicyListBox::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    // Header
    auto headerLayout = setupHeader();
    layout->addLayout(headerLayout);

    // Table View
    setupTableView();
    layout->addWidget(m_tableView);

    this->setLayout(layout);
}

QLayout *PolicyListBox::setupHeader()
{
    m_label = ControlUtil::createLabel();
    m_label->setFont(ControlUtil::fontDemiBold());

    m_btAddPolicy = ControlUtil::createFlatToolButton(
            ":/icons/add.png", [&] { emit addPolicy(listType()); });
    m_btRemovePolicy = ControlUtil::createFlatToolButton(
            ":/icons/delete.png", [&] { emit removePolicy(listType()); });
    m_btEditPolicy = ControlUtil::createFlatToolButton(
            ":/icons/pencil.png", [&] { emit editPolicy(listType()); });

    auto layout = new QHBoxLayout();
    layout->setSpacing(2);
    layout->addWidget(m_label, 1);
    layout->addWidget(m_btAddPolicy);
    layout->addWidget(m_btRemovePolicy);
    layout->addWidget(m_btEditPolicy);

    return layout;
}

void PolicyListBox::setupTableView()
{
    m_tableView = new TableView();
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_tableView->horizontalHeader()->setVisible(false);

    m_tableView->setModel(listModel());
}
