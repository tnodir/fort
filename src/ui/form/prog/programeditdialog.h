#ifndef PROGRAMEDITDIALOG_H
#define PROGRAMEDITDIALOG_H

#include <form/controls/formwindow.h>

class App;
class ProgramEditController;
class ProgMainPage;

class ProgramEditDialog : public FormWindow
{
    Q_OBJECT

public:
    explicit ProgramEditDialog(
            ProgramEditController *ctrl, QWidget *parent = nullptr, Qt::WindowFlags f = {});

    ProgramEditController *ctrl() const { return m_ctrl; }

    void initialize(const App &app, const QVector<qint64> &appIdList = {});

    bool isNew() const;

protected:
    virtual void closeOnSave() { close(); }

protected slots:
    void retranslateUi();
    virtual void retranslateWindowTitle();

private:
    void setupController();

    void setupUi();

private:
    ProgramEditController *m_ctrl = nullptr;

    ProgMainPage *m_mainPage = nullptr;
};

#endif // PROGRAMEDITDIALOG_H
