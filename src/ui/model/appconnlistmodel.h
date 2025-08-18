#ifndef APPCONNLISTMODEL_H
#define APPCONNLISTMODEL_H

#include <QVector>

#include "connlistmodel.h"

class AppConnListModel : public ConnListModel
{
    Q_OBJECT

public:
    explicit AppConnListModel(QObject *parent = nullptr);

    qint64 confAppId() const { return m_confAppId; }
    void setConfAppId(qint64 v) { m_confAppId = v; }

    const QString &appPath() const { return m_appPath; }
    void setAppPath(const QString &v) { m_appPath = v; }

protected:
    void fillConnIdRange(qint64 &idMin, qint64 &idMax) override;

    bool isConnIdRangeOut(
            qint64 oldIdMin, qint64 oldIdMax, qint64 idMin, qint64 idMax) const override;

    qint64 connIdByIndex(int row) const override;

    int doSqlCount() const override { return m_connIds.size(); }

private:
    qint64 m_confAppId;
    QString m_appPath;

    QVector<qint64> m_connIds;
};

#endif // APPCONNLISTMODEL_H
