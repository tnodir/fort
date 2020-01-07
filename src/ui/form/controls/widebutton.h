#ifndef WIDEBUTTON_H
#define WIDEBUTTON_H

#include <QPushButton>

class WideButton : public QPushButton
{
    Q_OBJECT

public:
    explicit WideButton(QWidget *parent = nullptr);
    explicit WideButton(const QIcon &icon, const QString &text = QString(),
                        QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;
};

#endif // WIDEBUTTON_H
