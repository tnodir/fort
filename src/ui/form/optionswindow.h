#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include <QObject>
#include <QWindow>

class OptionsWindow : public QWindow
{
    Q_OBJECT

public:
    explicit OptionsWindow(QWindow *parent = nullptr);

signals:

public slots:
    void retranslateUi();

private:
    void setupUi();
};

#endif // OPTIONSWINDOW_H
