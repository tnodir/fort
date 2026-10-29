#ifndef FORMPOINTER_H
#define FORMPOINTER_H

#include <QObject>

#include <memory>

#include <form/controls/formwindow.h>

class FormPointer
{
public:
    FormPointer(WindowCode code = WindowNone);

    WindowCode code() const { return m_code; }
    FormWindow *window() const { return m_window.get(); }

    FormWindow *initialize();

    bool show(bool activate = true, bool expose = false);
    bool close(bool isAppQuitting = false);

private:
    WindowCode m_code = WindowNone;

    std::unique_ptr<FormWindow> m_window;
};

#endif // FORMPOINTER_H
