#ifndef FILTEREDITDIALOG_H
#define FILTEREDITDIALOG_H

#include <QDialog>

#include <util/conf/filterlinetext.h>

QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QFormLayout)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

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
    void initialize(const FilterLineText &lineText = {}, int row = -1);

signals:
    void filterSaved(const QString &filterText, int row);

private:
    void retranslateUi();
    void retranslateNotButtons();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupFormLayout();
    QLayout *setupActionsLayout();
    QRadioButton *createActionButton(int actionTypeId);
    QCheckBox *createFieldCheckBox();
    QToolButton *createNotButton();
    QToolButton *createEqualButton(LineEdit *edit);
    LineEdit *createValuesEdit();
    PlainTextEdit *createValuesArea();
    QLabel *addWidgetsRow(
            QFormLayout *layout, const QString &labelText, const QList<QWidget *> &widgets);
    QLabel *addAreaRow(QFormLayout *layout, const QString &labelText,
            const QList<QWidget *> &widgets, QWidget *area);
    QLayout *setupOptionsLayout();
    void setupTextBox();
    QLayout *setupButtons();

    void updateTextByFields();
    void updateClearButtons();

    QString filterText() const;
    void pasteText();
    void setFilterLine(const FilterLineText &lineText);
    void setActionFilter(const RuleFilter *filter);

    QList<QCheckBox *> directionCheckBoxes() const;
    void setDirectionFilter(const RuleFilter *filter);

    QList<QCheckBox *> protocolCheckBoxes() const;
    QString protocolValues() const;
    void setProtocolFilter(const RuleFilter *filter);

    QList<QCheckBox *> areasCheckBoxes() const;
    void setAreasFilter(const RuleFilter *filter);

    QList<QCheckBox *> optionCheckBoxes() const;
    void setOptionFilter(const RuleFilter *filter);

    bool save();
    bool validateEditText() const;
    bool validateText(const QString &text) const;

private:
    bool m_isRuleFilter = false;

    int m_row = -1;

    QLabel *m_labelAction = nullptr;
    QButtonGroup *m_buttonGroupAction = nullptr;
    QRadioButton *m_rbRuleAction = nullptr;
    QRadioButton *m_rbAllow = nullptr;
    QRadioButton *m_rbBlock = nullptr;
    QRadioButton *m_rbAsk = nullptr;
    QRadioButton *m_rbDrop = nullptr;
    QLabel *m_labelDirection = nullptr;
    QToolButton *m_btDirectionNot = nullptr;
    QCheckBox *m_cbDirectionIn = nullptr;
    QCheckBox *m_cbDirectionOut = nullptr;
    QLabel *m_labelProtocols = nullptr;
    QToolButton *m_btProtocolsNot = nullptr;
    QCheckBox *m_cbProtocolTcp = nullptr;
    QCheckBox *m_cbProtocolUdp = nullptr;
    LineEdit *m_editProtocols = nullptr;
    QLabel *m_labelAreas = nullptr;
    QToolButton *m_btAreasNot = nullptr;
    QCheckBox *m_cbAreaLocalhost = nullptr;
    QCheckBox *m_cbAreaLan = nullptr;
    QCheckBox *m_cbAreaInet = nullptr;
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
    QToolButton *m_btLocalIpsEqual = nullptr;
    QLabel *m_labelLocalPorts = nullptr;
    QToolButton *m_btLocalPortsNot = nullptr;
    LineEdit *m_editLocalPorts = nullptr;
    QToolButton *m_btLocalPortsEqual = nullptr;
    QLabel *m_labelOptions = nullptr;
    QCheckBox *m_cbOptionLog = nullptr;
    QCheckBox *m_cbOptionNoLog = nullptr;
    QCheckBox *m_cbOptionAlert = nullptr;
    QGroupBox *m_gbText = nullptr;
    PlainTextEdit *m_editText = nullptr;
    QToolButton *m_btCopyText = nullptr;
    QToolButton *m_btPasteText = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;
};

#endif // FILTEREDITDIALOG_H
