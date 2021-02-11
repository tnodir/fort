#include "connlistmodel.h"

ConnListModel::ConnListModel(QObject *parent) : StringListModel(parent) { }

void ConnListModel::setAppPath(const QString &appPath)
{
    m_appPath = appPath;
}

void ConnListModel::clear()
{
    m_appPath = QString();

    StringListModel::clear();
}
