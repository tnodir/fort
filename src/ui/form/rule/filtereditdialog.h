#ifndef FILTEREDITDIALOG_H
#define FILTEREDITDIALOG_H

#include <QDialog>

#include <util/conf/filterlinetext.h>

QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFormLayout)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class LineEdit;
class PlainTextEdit;
class RuleFilter;
class SpinCombo;

class FilterEditDialog : public QDialog
{
    Q_OBJECT

public:
    // The Rule's filter may have no action: the Rule's one is applied
    explicit FilterEditDialog(bool isRuleFilter, QWidget *parent = nullptr);

    bool isRuleFilter() const { return m_isRuleFilter; }

    bool isEmpty() const { return m_row < 0; }

    // The row < 0 adds a new filter
    void initialize(const FilterLineText &lineText = {}, int row = -1);

signals:
    void filterSaved(const QString &filterText, int row);

private:
    void retranslateUi();
    void retranslateProtocolNames();
    void retranslateNotButtons();
    void retranslateClearButtons();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupFormLayout();
    QLayout *setupActionsLayout();
    QRadioButton *createActionButton(const QString &iconPath, int actionId);
    QComboBox *createFieldCombo(const QStringList &values);
    SpinCombo *createProtocolSpinCombo();
    QCheckBox *createFieldCheckBox();
    QToolButton *createNotButton();
    LineEdit *createValuesEdit();
    PlainTextEdit *createValuesArea();
    QToolButton *createComboClearButton(QComboBox *combo);
    QLabel *addWidgetsRow(
            QFormLayout *layout, const QString &labelText, const QList<QWidget *> &widgets);
    QLabel *addAreaRow(QFormLayout *layout, const QString &labelText,
            const QList<QWidget *> &widgets, QWidget *area);
    QLabel *addNotRow(
            QFormLayout *layout, const QString &labelText, QToolButton *btNot, QWidget *field);
    QLayout *setupOptionsLayout();
    void setupTextBox();
    QLayout *setupButtons();

    void updateTextByFields();

    QString filterText() const;
    void setFilterLine(const FilterLineText &lineText);
    void setActionFilter(const RuleFilter *filter);

    QList<QCheckBox *> optionCheckBoxes() const;
    QString optionValues() const;
    void setOptionFilter(const RuleFilter *filter);

    bool save();
    bool validateEditText() const;

private:
    bool m_isRuleFilter = false;

    int m_row = -1;

    QLabel *m_labelAction = nullptr;
    QButtonGroup *m_buttonGroupAction = nullptr;
    QRadioButton *m_rbRuleAction = nullptr;
    QRadioButton *m_rbAllow = nullptr;
    QRadioButton *m_rbBlock = nullptr;
    QRadioButton *m_rbDrop = nullptr;
    QLabel *m_labelDirection = nullptr;
    QToolButton *m_btDirectionNot = nullptr;
    QComboBox *m_comboDirection = nullptr;
    QToolButton *m_btDirectionClear = nullptr;
    QLabel *m_labelProtocol = nullptr;
    QToolButton *m_btProtocolNot = nullptr;
    SpinCombo *m_scProtocol = nullptr;
    QToolButton *m_btProtocolClear = nullptr;
    QLabel *m_labelArea = nullptr;
    QToolButton *m_btAreaNot = nullptr;
    QComboBox *m_comboArea = nullptr;
    QToolButton *m_btAreaClear = nullptr;
    QLabel *m_labelRemoteIps = nullptr;
    QToolButton *m_btRemoteIpsNot = nullptr;
    QToolButton *m_btRemoteIpsClear = nullptr;
    PlainTextEdit *m_editRemoteIps = nullptr;
    QLabel *m_labelRemotePorts = nullptr;
    QToolButton *m_btRemotePortsNot = nullptr;
    LineEdit *m_editRemotePorts = nullptr;
    QLabel *m_labelLocalIps = nullptr;
    QToolButton *m_btLocalIpsNot = nullptr;
    LineEdit *m_editLocalIps = nullptr;
    QLabel *m_labelLocalPorts = nullptr;
    QToolButton *m_btLocalPortsNot = nullptr;
    LineEdit *m_editLocalPorts = nullptr;
    QLabel *m_labelOptions = nullptr;
    QCheckBox *m_cbOptionLog = nullptr;
    QCheckBox *m_cbOptionNoLog = nullptr;
    QCheckBox *m_cbOptionAlert = nullptr;
    QGroupBox *m_gbText = nullptr;
    PlainTextEdit *m_editText = nullptr;
    QToolButton *m_btCopyText = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;
};

#endif // FILTEREDITDIALOG_H
