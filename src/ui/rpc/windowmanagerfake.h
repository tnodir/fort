#ifndef WINDOWMANAGERFAKE_H
#define WINDOWMANAGERFAKE_H

#include <manager/windowmanager.h>

class WindowManagerFake : public WindowManager
{
    Q_OBJECT

public:
    explicit WindowManagerFake(QObject *parent = nullptr);

    void setUp() override { }
    void tearDown() override { }

    void initialize() override { }

public slots:
    bool exposeHomeWindow() override;

    bool showProgramEditForm(const QString &appPath) override;

    bool setWindowVisibleByCode(
            WindowCode code, const QVariant &visibleOrSwitch = {}, bool activate = true) override;

    bool checkPassword(WindowCode code = WindowNone) override;

    void showErrorBox(const QString &text, const QString &title = QString(),
            QWidget *parent = nullptr) override;
    void showInfoBox(const QString &text, const QString &title = QString(),
            QWidget *parent = nullptr) override;
};

#endif // WINDOWMANAGERFAKE_H
