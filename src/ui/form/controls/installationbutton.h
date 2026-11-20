#ifndef INSTALLATIONBUTTON_H
#define INSTALLATIONBUTTON_H

#include <QIcon>

#include "toolbutton.h"

class InstallationButton : public ToolButton
{
    Q_OBJECT

public:
    explicit InstallationButton(QWidget *parent = nullptr);

public slots:
    void showInstallationWindow();

private:
    void setupUi();
    void setupIcon();
    void updateIcon();
    void retranslateUi();

private:
    QIcon m_iconNormal;
    QIcon m_iconGray;
};

#endif // INSTALLATIONBUTTON_H
