#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include "../../util/window/widgetwindow.h"

QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(MainPage)
QT_FORWARD_DECLARE_CLASS(OptionsController)

class OptionsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit OptionsWindow(FortManager *fortManager, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupController();

    void setupUi();

    OptionsController *ctrl() const { return m_ctrl; }

private:
    OptionsController *m_ctrl = nullptr;

    MainPage *m_mainPage = nullptr;
};

#endif // OPTIONSWINDOW_H
