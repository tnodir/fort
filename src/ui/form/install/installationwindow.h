#ifndef INSTALLATIONWINDOW_H
#define INSTALLATIONWINDOW_H

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QGroupBox)

class InstallationWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit InstallationWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowInstallation; }
    QString windowIconPath() const override { return ":/icons/server_components.png"; }
    QString helpSection() const override { return "installation-window"; }
    QString iniGroup() const override;

    bool hasService() const { return m_hasService; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void retranslateUi();
    void retranslateDriverMessage();
    void retranslateServiceMessage();
    void retranslatePortableMessage();
    void retranslateComboAutoRun();

    void setupUi();
    QLayout *setupColumn();
    void setupDriverBox();
    QLayout *setupDriverLabelLayout();
    void setupDriverIcon();
    QLayout *setupDriverButtonsLayout();
    void setupServiceBox();
    QLayout *setupServiceLabelLayout();
    QLayout *setupServiceButtonsLayout();
    void setupPortableBox();
    QLayout *setupPortableLabelLayout();
    QLayout *setupPortableButtonsLayout();
    void setupIntegrationBox();
    QLayout *setupIntegrationLayout();
    QLayout *setupAutoRunLayout();

    void updateIsUserAdmin();
    void updateIsPortable();
    void updateHasService();

    void setServiceInstalled(bool install);

private:
    bool m_hasService = false;

    QGroupBox *m_gbDriver = nullptr;
    QGroupBox *m_gbService = nullptr;
    QGroupBox *m_gbPortable = nullptr;
    QGroupBox *m_gbIntegration = nullptr;

    QLabel *m_iconDriver = nullptr;
    QLabel *m_labelDriverMessage = nullptr;
    QPushButton *m_btInstallDriver = nullptr;
    QPushButton *m_btRemoveDriver = nullptr;

    QLabel *m_iconService = nullptr;
    QLabel *m_labelServiceMessage = nullptr;
    QPushButton *m_btInstallService = nullptr;
    QPushButton *m_btRemoveService = nullptr;

    QLabel *m_iconPortable = nullptr;
    QLabel *m_labelPortableMessage = nullptr;
    QPushButton *m_btUninstallPortable = nullptr;

    QCheckBox *m_cbExplorerMenu = nullptr;
    QCheckBox *m_cbSoundsPanel = nullptr;
    QLabel *m_labelAutoRun = nullptr;
    QComboBox *m_comboAutoRun = nullptr;
};

#endif // INSTALLATIONWINDOW_H
