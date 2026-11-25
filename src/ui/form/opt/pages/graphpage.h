#ifndef GRAPHPAGE_H
#define GRAPHPAGE_H

#include "optbasepage.h"

class GraphOptionsArea;

class GraphPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit GraphPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    QString helpSection() const override { return "traffic-graph-tab"; }

public slots:
    void onResetToDefault() override;

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();

private:
    GraphOptionsArea *m_graphOptions = nullptr;
};

#endif // GRAPHPAGE_H
