#ifndef OPTIONSBUTTON_H
#define OPTIONSBUTTON_H

#include <QToolButton>

class TrayIcon;

class OptionsButton : public QToolButton
{
    Q_OBJECT

public:
    explicit OptionsButton(int tabIndex = 0, QWidget *parent = nullptr);

    TrayIcon *trayIcon() const;

public slots:
    void showOptionsWindow();

private:
    void setupUi();

private:
    int m_tabIndex = 0;
};

#endif // OPTIONSBUTTON_H
