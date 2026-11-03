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
    void addConnFilter(const ConnRow &connRow) override;

    AppConnsWindow(const App &app, QWidget *parent, Qt::WindowFlags f);
    AppConnsWindow(
            AppConnSearchModel *connListModel, const App &app, QWidget *parent, Qt::WindowFlags f);

    void hideAppName();

    /* The alert's window isn't held by the Window Manager */
    void setupAlertWindow();

    /* The alert's window keeps its state in the ini group */
    void restoreAlertWindowState(const QString &iniGroup);

    void updateProgramColumnHidden();

    bool logAllowedConn() const override;
    bool logBlockedConn() const override;

protected slots:
    void retranslateWindowTitle() override;

private:
    void setupUi();
    void setupEditAppName();
    void setupAddFilter();

    bool updateApp(const App &app);

private:
    bool m_appLogAllowedConn : 1 = true;
    bool m_appLogBlockedConn : 1 = true;

    LineEdit *m_editAppName = nullptr;
    QWidget *m_appNameSeparator = nullptr;
};

#endif // APPCONNSWINDOW_H
