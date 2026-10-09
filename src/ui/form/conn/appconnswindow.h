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
