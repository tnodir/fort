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
    void showErrorBox(const QString &text, const QString &title = QString()) override;
    void showInfoBox(const QString &text, const QString &title = QString()) override;
};

#endif // WINDOWMANAGERFAKE_H
