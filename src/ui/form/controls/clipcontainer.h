#ifndef CLIPCONTAINER_H
#define CLIPCONTAINER_H

#include <QWidget>

// Lays its widget out at least at the widget's minimum width and clips the rest: it can be
// narrower than the widget
class ClipContainer : public QWidget
{
    Q_OBJECT

public:
    explicit ClipContainer(QWidget *parent = nullptr);

    void setWidget(QWidget *w);
    QWidget *takeWidget();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateWidgetGeometry();

private:
    QWidget *m_widget = nullptr;
};

#endif // CLIPCONTAINER_H
