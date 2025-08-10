#ifndef PROGMOREPAGE_H
#define PROGMOREPAGE_H

#include "progbasepage.h"

class ProgMorePage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgMorePage(ProgramEditController *ctrl = nullptr, QWidget *parent = nullptr);

public slots:
    void fillApp(App &app) const override;

protected slots:
    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    void setupUi();
    void setupOptions();
    void setupLogConn();

private:
    QCheckBox *m_cbKillChild = nullptr;
    QCheckBox *m_cbParked = nullptr;
    QCheckBox *m_cbLogAllowedConn = nullptr;
    QCheckBox *m_cbLogBlockedConn = nullptr;
};

#endif // PROGMOREPAGE_H
