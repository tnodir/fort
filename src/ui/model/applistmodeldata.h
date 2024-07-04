#ifndef APPLISTMODELDATA_H
#define APPLISTMODELDATA_H

#include <QModelIndex>
#include <QObject>

class App;

class AppListModelData
{
public:
    explicit AppListModelData(
            const App &app, const QModelIndex &index = {}, int role = Qt::DisplayRole);

    int role() const { return m_role; }
    const QModelIndex &index() const { return m_index; }
    const App &app() const { return m_app; }

    int column() const { return index().column(); }

    QString appActionIconPath() const;
    QString appScheduleIconPath() const;

    QColor appActionColor() const;
    QVariant appGroupColor() const;

    QIcon appIcon() const;
    QIcon appZonesIcon() const;
    QIcon appRuleIcon() const;
    QIcon appScheduledIcon() const;
    QIcon appActionIcon() const;

    QVariant dataDecorationIcon() const;
    QVariant dataForeground() const;
    QVariant dataDisplayRow() const;

private:
    int m_role = Qt::DisplayRole;
    const QModelIndex &m_index;
    const App &m_app;
};

#endif // APPLISTMODELDATA_H
