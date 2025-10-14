#ifndef ZONEEDITDIALOG_H
#define ZONEEDITDIALOG_H

#include <QDialog>

#include <conf/zone.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

class LineEdit;
class PlainTextEdit;
class Zone;
class ZoneSourceWrapper;
class ZonesController;

class ZoneEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ZoneEditDialog(ZonesController *ctrl, QWidget *parent = nullptr);

    ZonesController *ctrl() const { return m_ctrl; }

    bool isEmpty() const { return m_zone.zoneId == 0; }

    void initialize(const Zone &zone);

signals:
    void saved();

protected slots:
    void retranslateUi();

private:
    void retranslateComboSources();

    void initializeBySource(const ZoneSourceWrapper &zoneSource);
    void initializeFocus();

    void setupController();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupNameLayout();
    void setupComboSources();
    void setupUrlFrame();
    QLayout *setupUrlLayout();
    void setupTextFrame();
    QLayout *setupTextLayout();
    QLayout *setupButtons();

    bool save();
    bool saveZone(Zone &zone);

    bool validateFields(const ZoneSourceWrapper &zoneSource) const;
    void fillZone(Zone &zone, const ZoneSourceWrapper &zoneSource) const;

private:
    ZonesController *m_ctrl = nullptr;

    QLabel *m_labelName = nullptr;
    LineEdit *m_editName = nullptr;
    QLabel *m_labelSource = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QFrame *m_frameUrl = nullptr;
    QCheckBox *m_cbCustomUrl = nullptr;
    QComboBox *m_comboSources = nullptr;
    QLabel *m_labelUrl = nullptr;
    LineEdit *m_editUrl = nullptr;
    QLabel *m_labelFormData = nullptr;
    LineEdit *m_editFormData = nullptr;
    QFrame *m_frameText = nullptr;
    PlainTextEdit *m_editText = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    Zone m_zone;
};

#endif // ZONEEDITDIALOG_H
