#ifndef ZONESWINDOW_H
#define ZONESWINDOW_H

#include "../../util/window/widgetwindow.h"

QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(TableView)
QT_FORWARD_DECLARE_CLASS(ZoneListModel)
QT_FORWARD_DECLARE_CLASS(ZonesController)

QT_FORWARD_DECLARE_STRUCT(ZoneRow)

class ZonesWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit ZonesWindow(FortManager *fortManager,
                         QWidget *parent = nullptr);

protected slots:
    void onSaveWindowState();
    void onRestoreWindowState();

    void onRetranslateUi();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupController();

    void setupUi();

    ZonesController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    ConfManager *confManager() const;
    ZoneListModel *zoneListModel() const;

private:
    bool m_formAppIsNew = false;

    ZonesController *m_ctrl = nullptr;
};

#endif // ZONESWINDOW_H
