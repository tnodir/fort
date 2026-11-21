#ifndef ACTIVITYWINDOW_H
#define ACTIVITYWINDOW_H

#include <form/controls/formwindow.h>

class ActivityController;
class ActivityTreeModel;
class HeaderButtons;
class TreeView;

class ActivityWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit ActivityWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowActivity; }
    QString windowIconPath() const override { return ":/icons/document_tree.png"; }
    QString helpSection() const override { return "activity"; }
    QString iniGroup() const override;

    ActivityController *ctrl() const { return m_ctrl; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected:
    // The model reads the DB only while the window is shown
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupTreeActivity();
    void setupTreeActivityHeader();
    void showTreeActivityHeaderMenu(const QPoint &pos);

private:
    ActivityController *m_ctrl = nullptr;

    HeaderButtons *m_headerButtons = nullptr;
    TreeView *m_treeView = nullptr;

    ActivityTreeModel *m_model = nullptr;
};

#endif // ACTIVITYWINDOW_H
