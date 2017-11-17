#ifndef IPLISTMODEL_H
#define IPLISTMODEL_H

#include "stringlistmodel.h"

class IpListModel : public StringListModel
{
    Q_OBJECT

public:
    explicit IpListModel(QObject *parent = nullptr);

    QString appPath() const { return m_appPath; }
    void setAppPath(const QString &appPath);

signals:

public slots:
    void clear() override;

private:
    QString m_appPath;
};

#endif // IPLISTMODEL_H
