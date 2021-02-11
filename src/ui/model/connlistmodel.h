#ifndef CONNLISTMODEL_H
#define CONNLISTMODEL_H

#include "../util/model/stringlistmodel.h"

class ConnListModel : public StringListModel
{
    Q_OBJECT

public:
    explicit ConnListModel(QObject *parent = nullptr);

    QString appPath() const { return m_appPath; }
    void setAppPath(const QString &appPath);

signals:

public slots:
    void clear() override;

private:
    QString m_appPath;
};

#endif // CONNLISTMODEL_H
