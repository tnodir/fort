#ifndef APPCONNLISTMODEL_H
#define APPCONNLISTMODEL_H

#include <QVector>

#include "connlistmodel.h"

class AppConnListModel : public ConnListModel
{
    Q_OBJECT

public:
    explicit AppConnListModel(QObject *parent = nullptr);

    const QString &appPath() const { return m_appPath; }
    void setAppPath(const QString &v) { m_appPath = v; }

protected:
    void fillConnIdRange(qint64 &idMin, qint64 &idMax) override;

    qint64 connIdByIndex(int row) const override;

    int doSqlCount() const override { return m_connIds.size(); }

private:
    QString m_appPath;

    QVector<qint64> m_connIds;
};

#endif // APPCONNLISTMODEL_H
