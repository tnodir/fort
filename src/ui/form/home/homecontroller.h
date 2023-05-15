#ifndef HOMECONTROLLER_H
#define HOMECONTROLLER_H

#include <form/basecontroller.h>

class HomeController : public BaseController
{
    Q_OBJECT

public:
    explicit HomeController(QObject *parent = nullptr);

    bool passwordLocked() const { return m_passwordLocked; }
    void setPasswordLocked(bool v);

signals:
    void afterSaveWindowState(IniUser *ini);
    void afterRestoreWindowState(IniUser *ini);

    void passwordLockedChanged();

private:
    void updatePasswordLocked();

private:
    bool m_passwordLocked = false;
};

#endif // HOMECONTROLLER_H
