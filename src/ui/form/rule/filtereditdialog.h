#ifndef FILTEREDITDIALOG_H
#define FILTEREDITDIALOG_H

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFormLayout)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class FilterLine;
class LineEdit;
class PlainTextEdit;
class RuleFilter;

class FilterEditDialog : public QDialog
{
    Q_OBJECT

public:
    // The Rule's filter may have no action: the Rule's one is applied
    explicit FilterEditDialog(bool isRuleFilter, QWidget *parent = nullptr);

    bool isRuleFilter() const { return m_isRuleFilter; }

    bool isEmpty() const { return m_row < 0; }

    // The row < 0 adds a new filter
    void initialize(const QString &filterText, int row = -1);

signals:
    void filterSaved(const QString &filterText, int row);

private:
    void retranslateUi();
    void retranslateNotCheckBoxes();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupFormLayout();
    QLayout *setupActionsLayout();
    QRadioButton *createActionButton(const QString &iconPath, int actionId);
    QComboBox *createFieldCombo(const QStringList &values);
    QCheckBox *createFieldCheckBox();
    LineEdit *createValuesEdit();
    PlainTextEdit *createValuesArea();
    QLabel *addWidgetsRow(
            QFormLayout *layout, const QString &labelText, const QList<QWidget *> &widgets);
    QLabel *addNotRow(
            QFormLayout *layout, const QString &labelText, QCheckBox *cbNot, QWidget *field);
    QLayout *setupOptionsLayout();
    void setupTextBox();
    QLayout *setupButtons();

    void updateTextByFields();
    void updateFieldsByText();

    QString filterText() const;
    void setFilterLine(const FilterLine &line);
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
    QCheckBox *m_cbDirectionNot = nullptr;
    QComboBox *m_comboDirection = nullptr;
    QLabel *m_labelProtocol = nullptr;
    QCheckBox *m_cbProtocolNot = nullptr;
    QComboBox *m_comboProtocol = nullptr;
    QLabel *m_labelArea = nullptr;
    QCheckBox *m_cbAreaNot = nullptr;
    QComboBox *m_comboArea = nullptr;
    QLabel *m_labelRemoteIps = nullptr;
    QCheckBox *m_cbRemoteIpsNot = nullptr;
    PlainTextEdit *m_editRemoteIps = nullptr;
    QLabel *m_labelRemotePorts = nullptr;
    QCheckBox *m_cbRemotePortsNot = nullptr;
    LineEdit *m_editRemotePorts = nullptr;
    QLabel *m_labelLocalIps = nullptr;
    QCheckBox *m_cbLocalIpsNot = nullptr;
    LineEdit *m_editLocalIps = nullptr;
    QLabel *m_labelLocalPorts = nullptr;
    QCheckBox *m_cbLocalPortsNot = nullptr;
    LineEdit *m_editLocalPorts = nullptr;
    QLabel *m_labelOptions = nullptr;
    QCheckBox *m_cbOptionLog = nullptr;
    QCheckBox *m_cbOptionNoLog = nullptr;
    QCheckBox *m_cbOptionAlert = nullptr;
    QGroupBox *m_gbText = nullptr;
    LineEdit *m_editText = nullptr;
    QToolButton *m_btCopyText = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;
};

#endif // FILTEREDITDIALOG_H
