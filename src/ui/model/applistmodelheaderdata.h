#ifndef APPLISTMODELHEADERDATA_H
#define APPLISTMODELHEADERDATA_H

#include <QObject>
#include <QVariant>

class AppListModelHeaderData
{
public:
    explicit AppListModelHeaderData(int column, int role = Qt::DisplayRole);

    int role() const { return m_role; }
    int column() const { return m_column; }

    QVariant headerDataDisplay() const;
    QVariant headerDataDecoration() const;

private:
    int m_role = Qt::DisplayRole;
    int m_column = 0;
};

#endif // APPLISTMODELHEADERDATA_H
