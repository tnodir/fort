#ifndef NATIVEWINDOW_H
#define NATIVEWINDOW_H

#include <QMainWindow>

class NativeWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit NativeWindow(QWidget *parent = nullptr);
};

#endif // NATIVEWINDOW_H
