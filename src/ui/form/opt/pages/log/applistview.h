#ifndef APPLISTVIEW_H
#define APPLISTVIEW_H

#include <QListView>

class AppListView : public QListView
{
    Q_OBJECT

public:
    explicit AppListView(QWidget *parent = nullptr);

private:
    void setupUi();
};

#endif // APPLISTVIEW_H
