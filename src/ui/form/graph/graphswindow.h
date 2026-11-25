#ifndef GRAPHSWINDOW_H
#define GRAPHSWINDOW_H

#include <form/controls/formwindow.h>

class GraphListModel;
class GraphsController;
class HeaderButtons;
class TableView;

class GraphsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit GraphsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowGraphs; }
    QString windowIconPath() const override { return ":/icons/action_log.png"; }
    QString helpSection() const override { return "traffic-graph"; }
    QString iniGroup() const override;

    GraphsController *ctrl() const { return m_ctrl; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupTableGraphs();
    void setupTableGraphsHeader();
    void setupTableGraphsChanged();
    void setupGraphListModelChanged();

    void addNewGraph();
    void editSelectedGraph();

    void openGraphEditForm(int graphId);

    void removeSelectedGraph();

    int graphListCurrentIndex() const;

private:
    GraphsController *m_ctrl = nullptr;
    GraphListModel *m_graphListModel = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddGraph = nullptr;
    QAction *m_actEditGraph = nullptr;
    QAction *m_actRemoveGraph = nullptr;
    HeaderButtons *m_headerButtons = nullptr;
    TableView *m_graphListView = nullptr;
};

#endif // GRAPHSWINDOW_H
