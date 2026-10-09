#ifndef FILTERSIMWINDOW_H
#define FILTERSIMWINDOW_H

#include <form/controls/formwindow.h>

#include <conf/filtersimconn.h>

QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QSpinBox)

class FilterSimController;
class LineEdit;
class ProtocolSelector;

class FilterSimWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit FilterSimWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowFilterSim; }
    QString windowOverlayIconPath() const override { return ":/icons/filter.png"; }

    FilterSimController *ctrl() const { return m_ctrl; }

    // Fills the input by the connection and simulates it
    void initialize(const FilterSimConn &simConn);

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();
    void retranslateComboDirection();
    void retranslateComboProfile();

    void setupUi();
    QLayout *setupConnLayout();
    QLayout *setupAppPathLayout();
    QLayout *setupRemoteLayout();
    QLayout *setupLocalLayout();
    QLayout *setupResultLayout();
    QLayout *setupResultActionLayout();

    void simulateConn();
    void simulateConnNow();

    bool fillSimConn(FilterSimConn &simConn) const;
    bool fillConnAddresses(FORT_CONF_META_CONN &conn) const;

    void updateResult();
    void clearResult();

    void showInputError(const QString &text) const;

private:
    bool m_simulated = false;

    FilterSimController *m_ctrl = nullptr;

    QGroupBox *m_gbConn = nullptr;
    QLabel *m_labelAppPath = nullptr;
    LineEdit *m_editAppPath = nullptr;
    QToolButton *m_btSelectFile = nullptr;
    QLabel *m_labelDirection = nullptr;
    QComboBox *m_comboDirection = nullptr;
    QLabel *m_labelProtocol = nullptr;
    ProtocolSelector *m_protocolSelector = nullptr;
    QLabel *m_labelRemoteIp = nullptr;
    LineEdit *m_editRemoteIp = nullptr;
    QLabel *m_labelRemotePort = nullptr;
    QSpinBox *m_spinRemotePort = nullptr;
    QLabel *m_labelLocalIp = nullptr;
    LineEdit *m_editLocalIp = nullptr;
    QLabel *m_labelLocalPort = nullptr;
    QSpinBox *m_spinLocalPort = nullptr;
    QCheckBox *m_cbLoopback = nullptr;
    QLabel *m_labelProfile = nullptr;
    QComboBox *m_comboProfile = nullptr;
    QPushButton *m_btSimulate = nullptr;
    QGroupBox *m_gbResult = nullptr;
    QLabel *m_labelAction = nullptr;
    QLabel *m_iconAction = nullptr;
    QLabel *m_resultAction = nullptr;
    QLabel *m_labelReason = nullptr;
    QLabel *m_resultReason = nullptr;
    QLabel *m_labelRule = nullptr;
    QLabel *m_resultRule = nullptr;
    QLabel *m_labelZone = nullptr;
    QLabel *m_resultZone = nullptr;
    QLabel *m_labelProgram = nullptr;
    QLabel *m_resultProgram = nullptr;
    QLabel *m_labelNote = nullptr;

    FilterSimConn m_simConn;
};

#endif // FILTERSIMWINDOW_H
