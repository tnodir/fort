#include "iplistmodel.h"

IpListModel::IpListModel(QObject *parent) :
    StringListModel(parent)
{
}

void IpListModel::setAppPath(const QString &appPath)
{
    m_appPath = appPath;
}

void IpListModel::clear()
{
    m_appPath = QString();

    StringListModel::clear();
}
