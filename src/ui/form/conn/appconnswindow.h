#ifndef APPCONNSWINDOW_H
#define APPCONNSWINDOW_H

#include "connectionswindow.h"

class App;
class AppConnSearchModel;
class LineEdit;

class AppConnsWindow : public ConnectionsWindow
{
    Q_OBJECT

public:
    explicit AppConnsWindow(const App &app, QWidget *parent = nullptr);

    AppConnSearchModel *appConnListModel() const;

    WindowCode windowCode() const override { return WindowNone; }

    void restoreWindowState() override;

protected slots:
    void retranslateWindowTitle() override;

private:
    void setupUi();
    void setupEditAppName();

    void updateApp(const App &app);
    void updateProgramColumnHidden();

private:
    LineEdit *m_editAppName = nullptr;
};

#endif // APPCONNSWINDOW_H
