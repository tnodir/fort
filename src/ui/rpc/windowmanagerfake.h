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

public slots:
    bool exposeHomeWindow() override;

    bool showProgramEditForm(const QString &appPath) override;

    bool checkPassword(bool temporary = false) override;

    void showErrorBox(const QString &text, const QString &title = QString(),
            QWidget *parent = nullptr) override;
    void showInfoBox(const QString &text, const QString &title = QString(),
            QWidget *parent = nullptr) override;
};

#endif // WINDOWMANAGERFAKE_H
