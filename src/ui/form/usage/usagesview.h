#ifndef USAGESVIEW_H
#define USAGESVIEW_H

#include <form/controls/treeview.h>

struct ConfUsages;

class UsagesModel;

class UsagesView : public TreeView
{
    Q_OBJECT

public:
    explicit UsagesView(QWidget *parent = nullptr);

    void setUsages(const ConfUsages &usages);
    void clearUsages();

    void retranslateUi();

signals:
    // Shown without the usages: the owner loads them
    void usagesRequested();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void openUsage(const QModelIndex &index);

private:
    bool m_usagesLoaded = false;

    UsagesModel *m_usagesModel = nullptr;
};

#endif // USAGESVIEW_H
