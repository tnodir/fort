#ifndef SERVICESPAGE_H
#define SERVICESPAGE_H

#include "optbasepage.h"

class TableView;

class ServicesPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit ServicesPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    QLayout *setupHeader();
    void setupOptions();
    void setupFilterServices();
    void setupTableServList();
    void setupTableServListHeader();

private:
    QPushButton *m_btOptions = nullptr;
    QCheckBox *m_cbFilterServices = nullptr;
    TableView *m_servListView = nullptr;
};

#endif // SERVICESPAGE_H
