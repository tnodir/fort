#ifndef SPEEDLIMITEDITDIALOG_H
#define SPEEDLIMITEDITDIALOG_H

#include <QDialog>

#include <conf/speedlimit.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QDoubleSpinBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QSpinBox)

class LineEdit;
class PlainTextEdit;
class SpeedLimitsController;
class SpinCombo;

class SpeedLimitEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpeedLimitEditDialog(SpeedLimitsController *ctrl, QWidget *parent = nullptr);

    SpeedLimitsController *ctrl() const { return m_ctrl; }

    bool isEmpty() const { return m_speedLimit.limitId == 0; }

    void initialize(const SpeedLimit &speedLimit);

protected slots:
    void retranslateUi();

private:
    void initializeFocus();

    void retranslateSpeedNames();

    void setupController();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupNameLayout();
    QLayout *setupDirectionLayout();
    QLayout *setupLimitLayout();
    QLayout *setupButtons();

    bool save();
    bool saveSpeedLimit(SpeedLimit &speedLimit);

    void fillSpeedLimit(SpeedLimit &speedLimit) const;

private:
    SpeedLimitsController *m_ctrl = nullptr;

    QLabel *m_labelName = nullptr;
    LineEdit *m_editName = nullptr;
    QLabel *m_labelNotes = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QRadioButton *m_rbDownload = nullptr;
    QRadioButton *m_rbUpload = nullptr;
    QLabel *m_labelSpeed = nullptr;
    SpinCombo *m_scSpeed = nullptr;
    QLabel *m_labelLatency = nullptr;
    QSpinBox *m_spinLatency = nullptr;
    QLabel *m_labelPacketLoss = nullptr;
    QDoubleSpinBox *m_spinPacketLoss = nullptr;
    QLabel *m_labelBufferSize = nullptr;
    QSpinBox *m_spinBufferSize = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    SpeedLimit m_speedLimit;
};

#endif // SPEEDLIMITEDITDIALOG_H
