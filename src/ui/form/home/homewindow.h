#ifndef HOMEWINDOW_H
#define HOMEWINDOW_H

#include <form/controls/formwindow.h>

class HeaderButtons;
class HomeController;
class HomeMainPage;

class HomeWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit HomeWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowHome; }
    QString helpSection() const override { return "my-fort-window"; }
    QString iniGroup() const override;

    HomeController *ctrl() const { return m_ctrl; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

public slots:
    void selectAboutTab();

protected slots:
    bool checkAboutToClose() override;

private slots:
    void onActivationChanged(bool isActive);

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QWidget *setupHeader();
    void setupHeaderButtons();
    void setupPasswordButtons();
    QLayout *setupDialogButtons();

    void updatePasswordButtons();

private:
    HomeController *m_ctrl = nullptr;

    HomeMainPage *m_mainPage = nullptr;
    QToolButton *m_btPasswordLock = nullptr;
    QToolButton *m_btPasswordUnlock = nullptr;
    HeaderButtons *m_headerButtons = nullptr;

    QList<QToolButton *> m_linkButtons;
};

#endif // HOMEWINDOW_H
