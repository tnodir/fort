#ifndef PASSWORDLOCKFRAME_H
#define PASSWORDLOCKFRAME_H

#include <QFrame>

// Disabled while the password is required
class PasswordLockFrame : public QFrame
{
    Q_OBJECT

public:
    explicit PasswordLockFrame(QWidget *parent = nullptr);

private:
    void updateLocked();
};

#endif // PASSWORDLOCKFRAME_H
