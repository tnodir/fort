#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include <form/controls/formwindow.h>

class OptMainPage;
class OptionsController;

class OptionsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit OptionsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowOptions; }
    QString windowOverlayIconPath() const override { return ":/icons/cog.png"; }

    OptionsController *ctrl() const { return m_ctrl; }

    void selectTab(int index);

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected slots:
    void cancelChanges();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();

    void checkDeprecated();
    void checkDeprecatedAppGroups();

private:
    OptionsController *m_ctrl = nullptr;

    OptMainPage *m_mainPage = nullptr;
};

#endif // OPTIONSWINDOW_H
