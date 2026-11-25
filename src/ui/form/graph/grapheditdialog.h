#ifndef GRAPHEDITDIALOG_H
#define GRAPHEDITDIALOG_H

#include <QDialog>

#include <user/iniuser.h>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

class GraphOptionsArea;
class GraphsController;
class IniGraph;
class LineEdit;

class GraphEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GraphEditDialog(GraphsController *ctrl, int graphId, QWidget *parent = nullptr);

    GraphsController *ctrl() const { return m_ctrl; }

protected slots:
    void retranslateUi();

private:
    IniGraph &iniGraph() const;

    void setupController();

    void setupUi(int graphId);
    QLayout *setupNameLayout();
    QLayout *setupButtons();

    void setEdited(bool edited = true);

    void save();

private:
    GraphsController *m_ctrl = nullptr;

    QLabel *m_labelName = nullptr;
    LineEdit *m_editName = nullptr;
    GraphOptionsArea *m_graphOptions = nullptr;
    QPushButton *m_btDefault = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btApply = nullptr;
    QPushButton *m_btCancel = nullptr;

    IniUser m_iniUser;
};

#endif // GRAPHEDITDIALOG_H
