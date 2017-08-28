#include "appgroup.h"

AppGroup::AppGroup(QObject *parent) :
    QObject(parent),
    m_enabled(true)
{
}

void AppGroup::setEnabled(bool enabled)
{
    if ((bool) m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged();
    }
}

void AppGroup::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}

void AppGroup::setBlockText(const QString &blockText)
{
    if (m_blockText != blockText) {
        m_blockText = blockText;
        emit blockTextChanged();
    }
}

void AppGroup::setAllowText(const QString &allowText)
{
    if (m_allowText != allowText) {
        m_allowText = allowText;
        emit allowTextChanged();
    }
}
