#ifndef APPCONNSWINDOW_H
#define APPCONNSWINDOW_H

#include "connectionswindow.h"

class App;
class AppConnSearchModel;
class Conn;
class LineEdit;

class AppConnsWindow : public ConnectionsWindow
{
    Q_OBJECT

public:
    explicit AppConnsWindow(const App &app, QWidget *parent = nullptr);

    AppConnSearchModel *appConnListModel() const;

    WindowCode windowCode() const override { return WindowNone; }

    void setApp(const App &app);

    void setAddFilterVisible(bool visible);

    void restoreWindowState() override;

signals:
    void addFilterRequested(const Conn &conn);

protected:
    AppConnsWindow(const App &app, QWidget *parent, Qt::WindowFlags f);
    AppConnsWindow(
            AppConnSearchModel *connListModel, const App &app, QWidget *parent, Qt::WindowFlags f);

    QAction *addFilterAction() const { return m_actAddFilter; }

    /* The alert's window isn't held by the Window Manager */
    void setupAlertWindow();

    /* The alert's window keeps its state in the ini group */
    void restoreAlertWindowState(const QString &iniGroup);

    void updateProgramColumnHidden();

protected slots:
    void retranslateWindowTitle() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    void setupEditAppName();
    void setupAddFilter();

    void updateApp(const App &app);

private:
    LineEdit *m_editAppName = nullptr;
    QAction *m_actAddFilter = nullptr;
};

#endif // APPCONNSWINDOW_H
