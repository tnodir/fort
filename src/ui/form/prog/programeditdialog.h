#ifndef PROGRAMEDITDIALOG_H
#define PROGRAMEDITDIALOG_H

#include <QPointer>

#include <form/controls/formwindow.h>

class App;
class AppConnsWindow;
class ProgramEditController;
class ProgMainPage;

class ProgramEditDialog : public FormWindow
{
    Q_OBJECT

public:
    explicit ProgramEditDialog(QWidget *parent = nullptr, Qt::WindowFlags f = {});

    ProgramEditController *ctrl() const { return m_ctrl; }

    void initialize(const App &app, const QVector<qint64> &appIdList = {});

    bool isNew() const;

protected:
    virtual void closeOnSave() { close(); }

    virtual AppConnsWindow *createConnsWindow();

    void hideEvent(QHideEvent *event) override;

protected slots:
    void retranslateUi();
    virtual void retranslateWindowTitle();

private:
    void setupController();

    void setupUi();
    void setupMainLayout();

    void switchConnsWindow(bool visible);
    void openConnsWindow();
    void closeConnsWindow();
    void updateConnsWindow();

private:
    ProgramEditController *m_ctrl = nullptr;

    ProgMainPage *m_mainPage = nullptr;

    QPointer<AppConnsWindow> m_connsWindow;
};

#endif // PROGRAMEDITDIALOG_H
